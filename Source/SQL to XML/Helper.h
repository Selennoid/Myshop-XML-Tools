#pragma once
#include <Windows.h>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <queue>
#include "imgui.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <cstring>
#include <cstdlib>
#include <map>
#include <sql.h>
#include <sqlext.h>
#include "rapidxml.hpp"

class Helper
{
    public:
        struct GoodsList
        {
            int item_index;
            std::string goods_name;
            int item_count;
            int item_class;
            std::string preview_x;
            std::string preview_y;
            std::string preview_z;
            std::string preview_d;
            int goods_list_code;
            int parents_list_code;
            GoodsList()
            {
                item_index = 0;
                item_count = 0;
                item_class = 0;
                preview_x = "NULL";
                preview_y = "NULL";
                preview_z = "NULL";
                preview_d = "NULL";
                goods_list_code = 0;
                parents_list_code = 0;
                goods_name = "NULL";
            };
        };
        struct Goods
        {
            int goods_code;
            std::string goods_name;
            std::string goods_desc;
            int goods_set_count;
            int goods_limit_use;
            int goods_limit_time;
            int goods_cash_price;
            int goods_shop_new;
            int goods_shop_popular;
            int goods_category;
            int goods_category0;
            int goods_category1;
            int goods_category2;
            int goods_char_level;
            int goods_char_sex;
            int goods_char_type;
            bool goods_issell;
            std::vector<Helper::GoodsList> list;
            Goods()
            {
                goods_code = 0;
                goods_name = "";
                goods_desc = "";
                goods_category = 0;
                goods_set_count = 0;
                goods_limit_use = 0;
                goods_limit_time = 0;
                goods_cash_price = 0;
                goods_shop_new = 0;
                goods_shop_popular = 0;
                goods_category0 = 0;
                goods_category1 = 0;
                goods_category2 = 0;
                goods_char_level = 0;
                goods_char_sex = 0;
                goods_char_type = 0;
                goods_issell = false;
                list.clear();
            };
        };

        Helper() noexcept;
        SQLHENV hEnv = SQL_NULL_HENV;
        SQLHDBC hDbc = SQL_NULL_HDBC;
        SQLRETURN ret;
        std::string ltrim(std::string s);
        std::string rtrim(std::string s);
        std::string trim(std::string s);
        std::unordered_map<std::string, std::string> parseDsnFile(const std::string& path);
        std::string buildConnStringFromMap(const std::unordered_map<std::string, std::string>& m, const std::string& uidOverride, const std::string& pwdOverride);
        bool connectDBFromDsnFile(const std::string& dsnFilePath, const std::string& uid = "", const std::string& pwd = "");
        std::string sqlToString(SQLCHAR* buf, SQLLEN ind);
        std::vector<GoodsList> LoadGoodsList(int goods_code);
        Goods LoadGoods(int goods_code);
        void exportToXML(const std::vector<Goods>& characterItems, const std::vector<Goods>& myCampItems, const std::string& outputFile);
        void ExportAllGoods();
        void save_xml_node(std::ostream& out, rapidxml::xml_node<>* node, int indent = 0);
};