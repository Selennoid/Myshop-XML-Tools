#include "Helper.h"
#include "gui.h"

Helper::Helper() noexcept
{
    tblGoodsVector.clear();
    tblGoodsListVector.clear();
    tblGoodsLimitVector.clear();
}

std::string Helper::escapeSingleQuotes(const std::string& input) noexcept
{
    std::string result;
    result.reserve(input.size());
    for (char c : input)
    {
        if (c == '\'') 
        {
            result.push_back('\'');
            result.push_back('\'');
        }
        else
            result.push_back(c);
    }
    return result;
}

bool Helper::insertFromXML(rapidxml::xml_node<>* node, int iOption, tbl_goods& goodsItem, tbl_goods_list& goodsItemList) noexcept
{
    auto getAttr = [&](rapidxml::xml_node<>* node, const char* name) -> const char*
    {
        rapidxml::xml_attribute<>* attr = node->first_attribute(name);
        return attr ? attr->value() : "";
    };
    if (iOption == 0)
    {
        auto it = std::find_if(tblGoodsVector.begin(), tblGoodsVector.end(), [&](const tbl_goods& item)
        {
            return item.goods_code == std::atoi(getAttr(node, "goods_code"));
        });
        if (it != tblGoodsVector.end())
            return false;
        goodsItem.goods_code = std::atoi(getAttr(node, "goods_code"));
        goodsItem.goods_name = escapeSingleQuotes(getAttr(node, "goods_name"));
        goodsItem.goods_desc = escapeSingleQuotes(getAttr(node, "goods_desc"));
        goodsItem.goods_limit_desc = escapeSingleQuotes(getAttr(node, "goods_limit_desc"));
        goodsItem.goods_set_count = std::atoi(getAttr(node, "goods_set_count"));
        goodsItem.goods_limit_use = std::atoi(getAttr(node, "goods_limit_use"));
        goodsItem.goods_cash_price = std::atoi(getAttr(node, "goods_cash_price"));
        goodsItem.goods_shop_new = std::atoi(getAttr(node, "goods_shop_new"));
        goodsItem.goods_shop_popular = std::atoi(getAttr(node, "goods_shop_popular"));
        goodsItem.goods_category = std::atoi(getAttr(node, "goods_category"));
        goodsItem.goods_category0 = std::atoi(getAttr(node, "goods_category0"));
        goodsItem.goods_category1 = std::atoi(getAttr(node, "goods_category1"));
        goodsItem.goods_category2 = std::atoi(getAttr(node, "goods_category2"));
        goodsItem.goods_char_level = std::atoi(getAttr(node, "goods_char_level"));
        goodsItem.goods_char_sex = std::atoi(getAttr(node, "goods_char_sex"));
        goodsItem.goods_char_type = std::atoi(getAttr(node, "goods_char_type"));
        goodsItem.goods_issell = (bool)std::atoi(getAttr(node, "goods_issell"));
        goodsItem.goods_created = getCurrentDateTime();
        return true;
    }
    else if (iOption == 1)
    {
        goodsItemList.item_index = std::atoi(getAttr(node, "item_index"));
        goodsItemList.goods_scode = std::atoi(getAttr(node, "item_index"));
        goodsItemList.item_count = std::atoi(getAttr(node, "item_count"));
        goodsItemList.item_class = std::atoi(getAttr(node, "item_class"));
        goodsItemList.preview_x = escapeSingleQuotes(getAttr(node, "preview_x"));
        goodsItemList.preview_y = escapeSingleQuotes(getAttr(node, "preview_y"));
        goodsItemList.preview_z = escapeSingleQuotes(getAttr(node, "preview_z"));
        goodsItemList.preview_d = escapeSingleQuotes(getAttr(node, "preview_d"));
        goodsItemList.parents_list_code = std::atoi(getAttr(node, "parents_list_code"));
        goodsItemList.goods_list_code = std::atoi(getAttr(node, "goods_list_code"));
        return true;
    }
    else if (iOption == 2)
    {
        auto it = std::find_if(tblGoodsVector.begin(), tblGoodsVector.end(), [&](const tbl_goods& item)
        {
            return item.goods_code == std::atoi(getAttr(node, "item_index"));
        });
        if (it != tblGoodsVector.end())
            return false;
        goodsItem.goods_code = std::atoi(getAttr(node, "item_index"));
        goodsItem.goods_name = escapeSingleQuotes(getAttr(node, "goods_name"));
        goodsItem.goods_issell = false;
        goodsItem.goods_created = getCurrentDateTime();
        return true;
    }
}

void Helper::populateVectorsFromXML() noexcept
{
    std::ifstream file("libcmgds_e.xml");
    if (!file) 
    {
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content(buffer.str());
    std::vector<char> xml_copy(content.begin(), content.end());
    xml_copy.push_back('\0');
    rapidxml::xml_document<> doc;
    doc.parse<0>(&xml_copy[0]);
    auto getAttr = [&](rapidxml::xml_node<>* node, const char* name) -> const char*
    {
        rapidxml::xml_attribute<>* attr = node->first_attribute(name);
        return attr ? attr->value() : "";
    };
    rapidxml::xml_node<>* root = doc.first_node("ROOT");
    if (root) 
    {
        rapidxml::xml_node<>* character = root->first_node("CHARACTER");
        if (character) 
        {
            gui::AddLog("Populating item vectors...");
            for (rapidxml::xml_node<>* goods = character->first_node("GOODS"); goods; goods = goods->next_sibling("GOODS")) 
            {
                tbl_goods g;
                tbl_goods_list g_list;
                if(insertFromXML(goods, 0, g, g_list))
                    tblGoodsVector.push_back(g);
                for (rapidxml::xml_node<>* goods_list = goods->first_node("GOODS_LIST"); goods_list; goods_list = goods_list->next_sibling("GOODS_LIST"))
                {
                    if(std::atoi(getAttr(goods_list, "item_index")) != std::atoi(getAttr(goods, "goods_code")))
                        if (insertFromXML(goods_list, 2, g, g_list))
                            tblGoodsVector.push_back(g);
                    if (insertFromXML(goods_list, 1, g, g_list))
                        tblGoodsListVector.push_back(g_list);
                }
            }
        }
    }
    for (auto& element : tblGoodsVector)
    {
        tbl_goods_limit g_limit;
        g_limit.goods_code = element.goods_code;
        g_limit.goods_limit_price = element.goods_cash_price;
        g_limit.default_display = element.goods_issell;
        tblGoodsLimitVector.push_back(g_limit);
    }
    gui::AddLog("Item vectors populated successfully!");
    gui::AddLog("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
}

void Helper::generateSQLFiles(int iOption) noexcept
{
    int iRowCount = 0;
    std::stringstream queryContents;
    std::ofstream sqlFile;
    switch (iOption)
    {
        case 0:
        {
            iRowCount = 0;
            sqlFile = std::ofstream("Output/1 - tbl_goods.sql", std::ios::out);
            if (!sqlFile) 
            {
                gui::AddLog("Failed to write 1 - tbl_goods.sql...");
                return;
            }
            queryContents.str("");
            queryContents << "USE [gmg_account]" << std::endl << "GO" << std::endl << "DELETE FROM tbl_goods_limit" << std::endl << "DELETE FROM tbl_goods_list" << std::endl << "DELETE FROM tbl_goods_recharge" << std::endl << "DELETE FROM tbl_1won_event_goods" << std::endl << "DELETE FROM tbl_coupon" << std::endl << "DELETE FROM tbl_discount_goods" << std::endl << "DELETE FROM tbl_event_goods_log" << std::endl << "DELETE FROM tbl_gacha_goods_log" << std::endl << "DELETE FROM tbl_goods_best_category" << std::endl << "DELETE FROM tbl_sell" << std::endl << "DELETE FROM tbl_goods" << std::endl;
            for (auto& element : tblGoodsVector)
            {
                queryContents << "INSERT INTO tbl_goods (goods_code, goods_name, goods_desc, goods_capacity, goods_category, goods_set_count, goods_item_count, goods_limit_use, goods_limit_time, goods_cash_price, goods_created, goods_shop_new, goods_shop_popular, goods_sellcount, goods_category0, goods_category1, goods_category2, goods_limit_desc, goods_char_level, goods_char_sex, goods_char_type, version_code, goods_issell, goods_image) VALUES(" << element.goods_code << ", " << "'"  << element.goods_name << "'"  << ", " << "'"  << element.goods_desc << "'"  << ", " << "'"  << element.goods_capacity << "'"  << ", " << element.goods_category << ", " << element.goods_set_count  << ", "  << element.goods_item_count  << ", " << element.goods_limit_use << ", " << element.goods_limit_time << ", " << element.goods_cash_price << ", " << "'"  << element.goods_created << "'"  << ", " << element.goods_shop_new << ", " << element.goods_shop_popular << ", " << element.goods_sellcount << ", " << element.goods_category0 << ", " << element.goods_category1 << ", " << element.goods_category2 << ", " << "'"  << element.goods_limit_desc << "'"  << ", " << element.goods_char_level << ", " << element.goods_char_sex << ", " << element.goods_char_type << ", " << element.version_code << ", " << element.goods_issell << ", " << "'"  << element.goods_image << "'"  << ");" << std::endl;
                if (iRowCount > 399)
                {
                    queryContents << "GO" << std::endl;
                    iRowCount = 0;
                }
                iRowCount++;
            }
            sqlFile << queryContents.str();
            gui::AddLog("File generated successfully!");
        }
        break;
        case 1:
        {
            iRowCount = 0;
            sqlFile = std::ofstream("Output/2 - tbl_goods_list.sql", std::ios::out);
            if (!sqlFile)
            {
                gui::AddLog("Failed to write 2 - tbl_goods_list.sql...");
                return;
            }
            queryContents.str("");
            for (auto& element : tblGoodsListVector)
            {
                queryContents << "INSERT INTO tbl_goods_list (goods_code, item_index, item_count, goods_scode, item_class, preview_x, preview_y, preview_z, preview_d, goods_list_code, parents_list_code, goods_list_limit) VALUES(" << element.item_index << ", " << element.item_index << ", " << element.item_count << ", " << element.goods_scode << ", " << element.item_class << ", " << "'" << element.preview_x << "'" << ", " << "'" << element.preview_y << "'" << ", " << "'" << element.preview_z << "'" << ", " << "'" << element.preview_d << "'" << ", " << element.goods_list_code << ", " << element.parents_list_code << ", "  << element.goods_list_limit  << ");" << std::endl;
                if (iRowCount > 399)
                {
                    queryContents << "GO" << std::endl;
                    iRowCount = 0;
                }
                iRowCount++;
            }
            sqlFile << queryContents.str();
            gui::AddLog("File generated successfully!");
        }
        break;
        case 2:
        {
            iRowCount = 0;
            sqlFile = std::ofstream("Output/3 - tbl_goods_limit.sql", std::ios::out);
            if (!sqlFile)
            {
                gui::AddLog("Failed to write 3 - tbl_goods_limit.sql...");
                return;
            }
            queryContents.str("");
            for (auto& element : tblGoodsLimitVector)
            {
                queryContents << "INSERT INTO tbl_goods_limit (goods_code, limit_code, goods_limit_price, default_display) VALUES(" << element.goods_code << ", " << element.limit_code << ", " << element.goods_limit_price << ", " << element.default_display << ");" << std::endl;
                if (iRowCount > 399)
                {
                    queryContents << "GO" << std::endl;
                    iRowCount = 0;
                }
                iRowCount++;
            }
            sqlFile << queryContents.str();
            gui::AddLog("File generated successfully!");
        }
        break;
    }
    if (sqlFile)
        sqlFile.close();
}

std::string Helper::getCurrentDateTime() noexcept
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now_c);
#else
    localtime_r(&now_c, &localTime);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}