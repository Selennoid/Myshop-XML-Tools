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
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    const char* query =
        "SELECT A.goods_name, B.item_index, B.item_count, B.item_class, "
        "B.preview_x, B.preview_y, B.preview_z, B.preview_d, "
        "B.parents_list_code, B.goods_list_code "
        "FROM gmg_account.dbo.tbl_goods AS A "
        "LEFT JOIN gmg_account.dbo.tbl_goods_list AS B ON B.item_index = A.goods_code "
        "WHERE A.goods_code = ?";
    SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &goods_code, 0, NULL);
    if (SQLExecute(hStmt) == SQL_SUCCESS) 
    {
        GoodsList gl{};
        SQLCHAR goods_name[256];
        SQLLEN ind_goods_name, ind;
        SQLBindCol(hStmt, 1, SQL_C_CHAR, goods_name, sizeof(goods_name), &ind_goods_name);
        SQLBindCol(hStmt, 2, SQL_C_SLONG, &gl.item_index, 0, &ind);
        SQLBindCol(hStmt, 3, SQL_C_SLONG, &gl.item_count, 0, &ind);
        SQLBindCol(hStmt, 4, SQL_C_SLONG, &gl.item_class, 0, &ind);
        SQLBindCol(hStmt, 5, SQL_C_SLONG, &gl.preview_x, 0, &ind);
        SQLBindCol(hStmt, 6, SQL_C_SLONG, &gl.preview_y, 0, &ind);
        SQLBindCol(hStmt, 7, SQL_C_SLONG, &gl.preview_z, 0, &ind);
        SQLBindCol(hStmt, 8, SQL_C_SLONG, &gl.preview_d, 0, &ind);
        SQLBindCol(hStmt, 9, SQL_C_SLONG, &gl.parents_list_code, 0, &ind);
        SQLBindCol(hStmt, 10, SQL_C_SLONG, &gl.goods_list_code, 0, &ind);
        while (SQLFetch(hStmt) == SQL_SUCCESS) 
        {
            gl.goods_name = sqlToString(goods_name, ind_goods_name);
            result.push_back(gl);
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

Helper::Goods Helper::LoadGoods(int goods_code)
{
    Goods g{};
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    const char* query =
        "SELECT goods_code, goods_name, goods_desc, goods_set_count, goods_limit_use, "
        "goods_limit_time, goods_cash_price, goods_shop_new, goods_shop_popular, "
        "goods_category, goods_category0, goods_category1, goods_category2, "
        "goods_char_level, goods_char_sex, goods_char_type, goods_issell "
        "FROM gmg_account.dbo.tbl_goods WHERE goods_code = ?";
    SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &goods_code, 0, NULL);
    if (SQLExecute(hStmt) == SQL_SUCCESS && SQLFetch(hStmt) == SQL_SUCCESS) 
    {
        SQLCHAR goods_name[256], goods_desc[512];
        SQLLEN ind;
        SQLGetData(hStmt, 1, SQL_C_SLONG, &g.goods_code, 0, &ind);
        SQLGetData(hStmt, 2, SQL_C_CHAR, goods_name, sizeof(goods_name), &ind);
        SQLGetData(hStmt, 3, SQL_C_CHAR, goods_desc, sizeof(goods_desc), &ind);
        SQLGetData(hStmt, 4, SQL_C_SLONG, &g.goods_set_count, 0, &ind);
        SQLGetData(hStmt, 5, SQL_C_SLONG, &g.goods_limit_use, 0, &ind);
        SQLGetData(hStmt, 6, SQL_C_SLONG, &g.goods_limit_time, 0, &ind);
        SQLGetData(hStmt, 7, SQL_C_DOUBLE, &g.goods_cash_price, 0, &ind);
        SQLGetData(hStmt, 8, SQL_C_SLONG, &g.goods_shop_new, 0, &ind);
        SQLGetData(hStmt, 9, SQL_C_SLONG, &g.goods_shop_popular, 0, &ind);
        SQLGetData(hStmt, 10, SQL_C_SLONG, &g.goods_category, 0, &ind);
        SQLGetData(hStmt, 11, SQL_C_SLONG, &g.goods_category0, 0, &ind);
        SQLGetData(hStmt, 12, SQL_C_SLONG, &g.goods_category1, 0, &ind);
        SQLGetData(hStmt, 13, SQL_C_SLONG, &g.goods_category2, 0, &ind);
        SQLGetData(hStmt, 14, SQL_C_SLONG, &g.goods_char_level, 0, &ind);
        SQLGetData(hStmt, 15, SQL_C_SLONG, &g.goods_char_sex, 0, &ind);
        SQLGetData(hStmt, 16, SQL_C_SLONG, &g.goods_char_type, 0, &ind);
        SQLGetData(hStmt, 17, SQL_C_SLONG, &g.goods_issell, 0, &ind);
        g.goods_name = (char*)goods_name;
        g.goods_desc = (char*)goods_desc;
        g.list = LoadGoodsList(goods_code);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
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
            Goods g = LoadGoods(goods_code);
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