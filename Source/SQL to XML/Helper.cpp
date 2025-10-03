#include "Helper.h"
#include "gui.h"


Helper::Helper() noexcept
{
}

std::string Helper::ltrim(std::string s) 
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}

std::string Helper::rtrim(std::string s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string Helper::trim(std::string s) 
{ 
    return ltrim(rtrim(s)); 
}

std::unordered_map<std::string, std::string> Helper::parseDsnFile(const std::string& path) 
{
    std::unordered_map<std::string, std::string> m;
    std::ifstream in(path);
    if (!in.is_open()) return m;
    std::string line;
    while (std::getline(in, line)) 
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string t = trim(line);
        if (t.empty()) continue;
        if (t[0] == ';' || t[0] == '#') continue;
        if (!t.empty() && t.front() == '[' && t.back() == ']') continue;
        auto eq = t.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(t.substr(0, eq));
        std::string value = trim(t.substr(eq + 1));
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::toupper(c); });
        m[key] = value;
    }
    return m;
}

std::string Helper::buildConnStringFromMap(const std::unordered_map<std::string, std::string>& m, const std::string& uidOverride, const std::string& pwdOverride) 
{
    std::ostringstream oss;
    bool first = true;
    for (const auto& kv : m) 
    {
        if (kv.second.empty()) continue;
        if (!first) oss << ';';
        oss << kv.first << '=' << kv.second;
        first = false;
    }
    if (!uidOverride.empty()) 
    {
        if (!first) oss << ';';
        oss << "UID=" << uidOverride;
        first = false;
    }
    else if (m.find("UID") == m.end() && m.find("USER") == m.end()) {}
    if (!pwdOverride.empty()) 
    {
        if (!first) oss << ';';
        oss << "PWD=" << pwdOverride;
        first = false;
    }
    return oss.str();
}

bool Helper::connectDBFromDsnFile(const std::string& dsnFilePath, const std::string& uid, const std::string& pwd)
{
    std::stringstream errMsg;
    auto m = parseDsnFile(dsnFilePath);
    if (m.empty()) 
    {
        errMsg << "Failed to read or parse DSN file: " << dsnFilePath << std::endl;
        gui::AddLog(errMsg.str().c_str());
        return false;
    }
    std::string connStr = buildConnStringFromMap(m, uid, pwd);
    if (connStr.empty()) 
    {
        errMsg.str("");
        errMsg << "Empty connection string built from DSN file." << std::endl;
        gui::AddLog(errMsg.str().c_str());
        return false;
    }
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (!SQL_SUCCEEDED(ret)) 
    {
        errMsg.str("");
        errMsg << "SQLAllocHandle(SQL_HANDLE_ENV) failed" << std::endl;
        gui::AddLog(errMsg.str().c_str());
        return false;
    }
    ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (!SQL_SUCCEEDED(ret)) 
    {
        errMsg.str("");
        errMsg << "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION) failed" << std::endl;
        gui::AddLog(errMsg.str().c_str());
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (!SQL_SUCCEEDED(ret)) 
    {
        errMsg.str("");
        errMsg << "SQLAllocHandle(SQL_HANDLE_DBC) failed" << std::endl;
        gui::AddLog(errMsg.str().c_str());
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }
    SQLCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen = 0;
    ret = SQLDriverConnectA(hDbc, NULL, reinterpret_cast<SQLCHAR*>(const_cast<char*>(connStr.c_str())), SQL_NTS, outConnStr, sizeof(outConnStr), &outConnStrLen, SQL_DRIVER_COMPLETE);

    if (SQL_SUCCEEDED(ret)) 
    {
        errMsg.str("");
        errMsg << "Connected!" << std::endl;
        gui::AddLog(errMsg.str().c_str());
        return true;
    }
    else 
    {
        SQLCHAR sqlState[6] = { 0 };
        SQLCHAR msg[1024] = { 0 };
        SQLINTEGER nativeError = 0;
        SQLSMALLINT msgLen = 0;
        SQLGetDiagRecA(SQL_HANDLE_DBC, hDbc, 1, sqlState, &nativeError, msg, sizeof(msg), &msgLen);
        errMsg.str("");
        errMsg << "SQLDriverConnect failed. SQLSTATE=" << sqlState << " err=" << nativeError << " msg=" << msg << std::endl;
        gui::AddLog(errMsg.str().c_str());
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }
}

std::string Helper::sqlToString(SQLCHAR* buf, SQLLEN ind) 
{
    if (ind == SQL_NULL_DATA) return "";
    return std::string((char*)buf, (size_t)ind);
}

std::vector<Helper::GoodsList> Helper::LoadGoodsList(int goods_code)
{
    std::vector<GoodsList> result;
    SQLHSTMT hStmt{};
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    const char* query =
        "SELECT 'Item', item_index, item_count, item_class, "
        "preview_x, preview_y, preview_z, preview_d, "
        "parents_list_code, goods_list_code "
        "FROM gmg_account.dbo.tbl_goods_list "
        "WHERE item_index = ?";
    SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &goods_code, 0, nullptr);
    auto retExec = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(retExec))
    {
        SQLCHAR state[6] = { 0 }, msg[1024] = { 0 };
        SQLINTEGER nativeErr = 0; SQLSMALLINT msgLen = 0;
        if (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, state, &nativeErr, msg, sizeof(msg), &msgLen) == SQL_SUCCESS)
        {
            std::ostringstream oss;
            oss << "SQLExecute failed in LoadGoodsList for goods_code=" << goods_code
                << " SQLSTATE=" << state
                << " nativeErr=" << nativeErr
                << " msg=" << msg;
            gui::AddLog(oss.str().c_str());
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return result;
    }
    SQLCHAR goods_name[256] = { 0 };
    SQLLEN ind_name = 0, ind = 0;
    long item_index = 0, parents_list_code = 0, goods_list_code = 0;
    short item_count = 0;
    unsigned char item_class = 0;
    short preview_x = 0, preview_y = 0, preview_z = 0, preview_d = 0;
    SQLBindCol(hStmt,  1, SQL_C_CHAR, goods_name, sizeof(goods_name), &ind_name);
    SQLBindCol(hStmt,  2, SQL_C_SLONG, &item_index, 0, &ind);
    SQLBindCol(hStmt,  3, SQL_C_SSHORT, &item_count, 0, &ind);
    SQLBindCol(hStmt,  4, SQL_C_UTINYINT, &item_class, 0, &ind);
    SQLBindCol(hStmt,  5, SQL_C_SSHORT, &preview_x, 0, &ind);
    SQLBindCol(hStmt,  6, SQL_C_SSHORT, &preview_y, 0, &ind);
    SQLBindCol(hStmt,  7, SQL_C_SSHORT, &preview_z, 0, &ind);
    SQLBindCol(hStmt,  8, SQL_C_SSHORT, &preview_d, 0, &ind);
    SQLBindCol(hStmt,  9, SQL_C_SLONG, &parents_list_code, 0, &ind);
    SQLBindCol(hStmt, 10, SQL_C_SLONG, &goods_list_code, 0, &ind);
    while (true)
    {
        auto retFetch = SQLFetch(hStmt);
        if (retFetch == SQL_NO_DATA) break;

        if (SQL_SUCCEEDED(retFetch))
        {
            GoodsList gl{};
            gl.goods_name = (ind_name != SQL_NULL_DATA) ? (char*)goods_name : "";
            gl.item_index = item_index;
            gl.item_count = item_count;
            gl.item_class = item_class;
            gl.preview_x = preview_x;
            gl.preview_y = preview_y;
            gl.preview_z = preview_z;
            gl.preview_d = preview_d;
            gl.parents_list_code = parents_list_code;
            gl.goods_list_code = goods_list_code;
            result.push_back(gl);
        }
        else
        {
            SQLCHAR state[6] = { 0 }, msg[1024] = { 0 };
            SQLINTEGER nativeErr = 0; SQLSMALLINT msgLen = 0;
            if (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, state, &nativeErr, msg, sizeof(msg), &msgLen) == SQL_SUCCESS)
            {
                std::ostringstream oss;
                oss << "SQLFetch failed in LoadGoodsList for goods_code=" << goods_code
                    << " SQLSTATE=" << state
                    << " nativeErr=" << nativeErr
                    << " msg=" << msg;
                gui::AddLog(oss.str().c_str());
            }
            break;
        }
    }
    SQLCloseCursor(hStmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

Helper::Goods Helper::LoadGoods(int goods_code)
{
    Goods g{};
    g.goods_code = goods_code;
    SQLHSTMT hStmt{};
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    const char* query =
        "SELECT A.goods_code, A.goods_name, A.goods_desc, A.goods_set_count, A.goods_limit_use, "
        "A.goods_limit_time, B.goods_limit_price, A.goods_shop_new, A.goods_shop_popular, "
        "A.goods_category, A.goods_category0, A.goods_category1, A.goods_category2, "
        "A.goods_char_level, A.goods_char_sex, A.goods_char_type, A.goods_issell, A.goods_limit_desc "
        "FROM gmg_account.dbo.tbl_goods AS A "
        "LEFT JOIN gmg_account.dbo.tbl_goods_limit AS B ON B.goods_code = A.goods_code "
        "WHERE A.goods_code = ?";
    SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &goods_code, 0, nullptr);
    auto retExec = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(retExec))
    {
        SQLCHAR state[6] = { 0 }, msg[1024] = { 0 };
        SQLINTEGER nativeErr = 0;
        SQLSMALLINT msgLen = 0;
        if (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, state, &nativeErr, msg, sizeof(msg), &msgLen) == SQL_SUCCESS)
        {
            std::ostringstream oss;
            oss << "SQLExecute failed in LoadGoods for goods_code=" << goods_code
                << " SQLSTATE=" << state
                << " nativeErr=" << nativeErr
                << " msg=" << msg;
            gui::AddLog(oss.str().c_str());
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return g;
    }
    SQLLEN ind_code = 0, ind_name = 0, ind_desc = 0, ind_ldesc = 0, ind_tmp = 0;
    long code = 0, set_count = 0, limit_use = 0, limit_time = 0, cash_price = 0;
    long shop_new = 0, shop_popular = 0, cat = 0, cat0 = 0, cat1 = 0, cat2 = 0;
    short char_level = 0, char_type = 0;
    unsigned char char_sex = 0;
    unsigned char isSell = 0;
    SQLCHAR name[128] = { 0 }, desc[500] = { 0 }, limitDesc[500] = { 0 };
    SQLBindCol(hStmt,  1, SQL_C_SLONG, &code, 0, &ind_code);
    SQLBindCol(hStmt,  2, SQL_C_CHAR, name, sizeof(name), &ind_name);
    SQLBindCol(hStmt,  3, SQL_C_CHAR, desc, sizeof(desc), &ind_desc);
    SQLBindCol(hStmt,  4, SQL_C_SLONG, &set_count, 0, &ind_tmp);
    SQLBindCol(hStmt,  5, SQL_C_UTINYINT, &limit_use, 0, &ind_tmp);
    SQLBindCol(hStmt,  6, SQL_C_UTINYINT, &limit_time, 0, &ind_tmp);
    SQLBindCol(hStmt,  7, SQL_C_SLONG, &cash_price, 0, &ind_tmp);
    SQLBindCol(hStmt,  8, SQL_C_UTINYINT, &shop_new, 0, &ind_tmp);
    SQLBindCol(hStmt,  9, SQL_C_UTINYINT, &shop_popular, 0, &ind_tmp);
    SQLBindCol(hStmt, 10, SQL_C_UTINYINT, &cat, 0, &ind_tmp);
    SQLBindCol(hStmt, 11, SQL_C_UTINYINT, &cat0, 0, &ind_tmp);
    SQLBindCol(hStmt, 12, SQL_C_UTINYINT, &cat1, 0, &ind_tmp);
    SQLBindCol(hStmt, 13, SQL_C_UTINYINT, &cat2, 0, &ind_tmp);
    SQLBindCol(hStmt, 14, SQL_C_SSHORT, &char_level, 0, &ind_tmp);
    SQLBindCol(hStmt, 15, SQL_C_UTINYINT, &char_sex, 0, &ind_tmp);
    SQLBindCol(hStmt, 16, SQL_C_SSHORT, &char_type, 0, &ind_tmp);
    SQLBindCol(hStmt, 17, SQL_C_BIT, &isSell, 0, &ind_tmp);
    SQLBindCol(hStmt, 18, SQL_C_CHAR, limitDesc, sizeof(limitDesc), &ind_ldesc);
    auto retFetch = SQLFetch(hStmt);
    if (SQL_SUCCEEDED(retFetch))
    {
        g.goods_code = (ind_code != SQL_NULL_DATA) ? code : 0;
        g.goods_name = (ind_name != SQL_NULL_DATA) ? (char*)name : "";
        g.goods_desc = (ind_desc != SQL_NULL_DATA) ? (char*)desc : "";
        g.goods_set_count = set_count;
        g.goods_limit_use = limit_use;
        g.goods_limit_time = limit_time;
        g.goods_limit_desc = (ind_ldesc != SQL_NULL_DATA) ? (char*)limitDesc : "";
        g.goods_cash_price = cash_price;
        g.goods_shop_new = shop_new;
        g.goods_shop_popular = shop_popular;
        g.goods_category = cat;
        g.goods_category0 = cat0;
        g.goods_category1 = cat1;
        g.goods_category2 = cat2;
        g.goods_char_level = char_level;
        g.goods_char_sex = char_sex;
        g.goods_char_type = char_type;
        g.goods_issell = isSell;
    }
    else if (retFetch != SQL_NO_DATA)
    {
        SQLCHAR state[6] = { 0 }, msg[1024] = { 0 };
        SQLINTEGER nativeErr = 0; SQLSMALLINT msgLen = 0;
        if (SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, state, &nativeErr, msg, sizeof(msg), &msgLen) == SQL_SUCCESS)
        {
            std::ostringstream oss;
            oss << "SQLFetch failed in LoadGoods for goods_code=" << goods_code
                << " SQLSTATE=" << state
                << " nativeErr=" << nativeErr
                << " msg=" << msg;
            gui::AddLog(oss.str().c_str());
        }
    }
    SQLCloseCursor(hStmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    g.list = LoadGoodsList(goods_code);
    return g;
}

void Helper::save_xml_node(std::ostream& out, rapidxml::xml_node<>* node, int indent) 
{
    for (int i = 0; i < indent; i++)
        out << "  ";
    switch (node->type()) 
    {
        case rapidxml::node_element: 
        {
            out << "<" << node->name();
            for (auto* attr = node->first_attribute(); attr; attr = attr->next_attribute()) 
            {
                out << " " << attr->name() << "=\"" << attr->value() << "\"";
            }
            if (node->first_node()) 
            {
                out << ">\n";
                for (auto* child = node->first_node(); child; child = child->next_sibling()) 
                {
                    save_xml_node(out, child, indent + 1);
                }
                for (int i = 0; i < indent; i++) 
                    out << "  ";
                out << "</" << node->name() << ">\n";
            }
            else 
            {
                out << "/>\n";
            }
            break;
        }
        case rapidxml::node_data:
            out << node->value() << "\n";
            break;
        case rapidxml::node_comment:
            out << "<!--" << node->value() << "-->\n";
            break;
        default:
            break;
    }
}

void Helper::exportToXML(const std::vector<Helper::Goods>& characterItems, const std::vector<Helper::Goods>& myCampItems, const std::string& outputFile) 
{
    rapidxml::xml_document<> doc;
    auto* root = doc.allocate_node(rapidxml::node_element, "ROOT");
    doc.append_node(root);
    auto addSection = [&](const char* name, const std::vector<Goods>& items) 
    {
        auto* section = doc.allocate_node(rapidxml::node_element, name);
        section->append_attribute(doc.allocate_attribute("count", doc.allocate_string(std::to_string(items.size()).c_str())));
        for (const auto& g : items) 
        {
            auto* goods = doc.allocate_node(rapidxml::node_element, "GOODS");
            goods->append_attribute(doc.allocate_attribute("goods_code", doc.allocate_string(std::to_string(g.goods_code).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_name", doc.allocate_string(g.goods_name.c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_desc", doc.allocate_string(g.goods_desc.c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_category", doc.allocate_string(std::to_string(g.goods_category).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_set_count", doc.allocate_string(std::to_string(g.goods_set_count).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_limit_use", doc.allocate_string(std::to_string(g.goods_limit_use).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_limit_time", doc.allocate_string(std::to_string(g.goods_limit_time).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_cash_price", doc.allocate_string(std::to_string(g.goods_cash_price).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_shop_new", doc.allocate_string(std::to_string(g.goods_shop_new).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_shop_popular", doc.allocate_string(std::to_string(g.goods_shop_popular).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_category0", doc.allocate_string(std::to_string(g.goods_category0).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_category1", doc.allocate_string(std::to_string(g.goods_category1).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_category2", doc.allocate_string(std::to_string(g.goods_category2).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_char_level", doc.allocate_string(std::to_string(g.goods_char_level).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_char_sex", doc.allocate_string(std::to_string(g.goods_char_sex).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_char_type", doc.allocate_string(std::to_string(g.goods_char_type).c_str())));
            goods->append_attribute(doc.allocate_attribute("goods_issell", doc.allocate_string(std::to_string(g.goods_issell).c_str())));
            for (const auto& l : g.list) 
            {
                auto* list = doc.allocate_node(rapidxml::node_element, "GOODS_LIST");
                list->append_attribute(doc.allocate_attribute("item_index", doc.allocate_string(std::to_string(l.item_index).c_str())));
                list->append_attribute(doc.allocate_attribute("goods_name", doc.allocate_string(l.goods_name.c_str())));
                list->append_attribute(doc.allocate_attribute("item_count", doc.allocate_string(std::to_string(l.item_count).c_str())));
                list->append_attribute(doc.allocate_attribute("item_class", doc.allocate_string(std::to_string(l.item_class).c_str())));
                list->append_attribute(doc.allocate_attribute("preview_x", doc.allocate_string(l.preview_x.c_str())));
                list->append_attribute(doc.allocate_attribute("preview_y", doc.allocate_string(l.preview_y.c_str())));
                list->append_attribute(doc.allocate_attribute("preview_z", doc.allocate_string(l.preview_z.c_str())));
                list->append_attribute(doc.allocate_attribute("preview_d", doc.allocate_string(l.preview_d.c_str())));
                list->append_attribute(doc.allocate_attribute("goods_list_code", doc.allocate_string(std::to_string(l.goods_list_code).c_str())));
                list->append_attribute(doc.allocate_attribute("parents_list_code", doc.allocate_string(std::to_string(l.parents_list_code).c_str())));
                goods->append_node(list);
            }
            section->append_node(goods);
        }
        root->append_node(section);
    };
    addSection("CHARACTER", characterItems);
    addSection("MYCAMP", myCampItems);
    auto* popup = doc.allocate_node(rapidxml::node_element, "POPUP");
    root->append_node(popup);
    std::ofstream out(outputFile);
    if (out.is_open()) 
    {
        out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        for (auto* node = doc.first_node(); node; node = node->next_sibling()) 
        {
            save_xml_node(out, node);
        }
    }
    out.close();
}

void Helper::ExportAllGoods() 
{
    std::vector<Goods> characterItems;
    std::vector<Goods> myCampItems;
    std::vector<int> codes;
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    const char* query =
        "SELECT goods_code FROM gmg_account.dbo.tbl_goods "
        "WHERE goods_issell = 1 AND goods_category0 < 7";
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query, SQL_NTS) == SQL_SUCCESS) 
    {
        int goods_code{};
        SQLLEN ind;
        SQLBindCol(hStmt, 1, SQL_C_SLONG, &goods_code, 0, &ind);
        gui::AddLog("Reading tbl_goods table...");
        while (SQLFetch(hStmt) == SQL_SUCCESS) 
        {
            while (SQLFetch(hStmt) == SQL_SUCCESS) 
            {
                codes.push_back(goods_code);
            }
            SQLCloseCursor(hStmt);
        }
        for (int code : codes)
        {
            Goods g = LoadGoods(code);
            if (g.goods_code == 0) continue;
            if (g.goods_category == 0)
                characterItems.push_back(g);
            else
                myCampItems.push_back(g);
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    gui::AddLog("Generating libcmgds_e.xml file...");
    exportToXML(characterItems, myCampItems, "libcmgds_e.xml");
}