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
#include "rapidxml.hpp"

class Helper
{
    public:
        struct tbl_goods
        {
            int goods_code;
            std::string goods_name;
            std::string goods_desc;
            std::string goods_capacity;
            int goods_category;
            int goods_set_count;
            std::string goods_item_count;
            int goods_limit_use;
            int goods_limit_time;
            int goods_cash_price;
            std::string goods_created;
            int goods_shop_new;
            int goods_shop_popular;
            int goods_sellcount;
            int goods_category0;
            int goods_category1;
            int goods_category2;
            std::string goods_limit_desc;
            int goods_char_level;
            int goods_char_sex;
            int goods_char_type;
            int version_code;
            bool goods_issell;
            std::string goods_image;
            tbl_goods()
            {
                goods_code = 0;
                goods_name = "";
                goods_desc = "";
                goods_capacity = "";
                goods_category = 0;
                goods_set_count = 0;
                goods_item_count = "NULL";
                goods_limit_use = 0;
                goods_limit_time = 0;
                goods_cash_price = 0;
                goods_created = "2025-01-01 00:00:00";
                goods_shop_new = 0;
                goods_shop_popular = 0;
                goods_sellcount = 0;
                goods_category0 = 0;
                goods_category1 = 0;
                goods_category2 = 0;
                goods_limit_desc = "";
                goods_char_level = 0;
                goods_char_sex = 0;
                goods_char_type = 0;
                version_code = 10001;
                goods_issell = false;
                goods_image = "abc";
            };
        };
        struct tbl_goods_list
        {
            int goods_code;
            int item_index;
            int item_count;
            int goods_scode;
            int item_class;
            std::string preview_x;
            std::string preview_y;
            std::string preview_z;
            std::string preview_d;
            int goods_list_code;
            int parents_list_code;
            std::string goods_list_limit;
            tbl_goods_list()
            {
                goods_code = 0;
                item_index = 0;
                item_count = 0;
                goods_scode = 0;
                item_class = 0;
                preview_x = "NULL";
                preview_y = "NULL";
                preview_z = "NULL";
                preview_d = "NULL";
                goods_list_code = 0;
                parents_list_code = 0;
                goods_list_limit = "NULL";
            };
        };
        struct tbl_goods_limit
        {
            int goods_code;
            int limit_code;
            int goods_limit_price;
            bool default_display;
            tbl_goods_limit()
            {
                goods_code = 0;
                limit_code = 0;
                goods_limit_price = 0;
                default_display = false;
            };
        };
        Helper() noexcept;
        std::string escapeSingleQuotes(const std::string& input) noexcept;
        void generateSQLFiles(int iOption) noexcept;
        void populateVectorsFromXML() noexcept;
        std::string getCurrentDateTime() noexcept;
        std::vector<tbl_goods> tblGoodsVector;
        std::vector<tbl_goods_list> tblGoodsListVector;
        std::vector<tbl_goods_limit> tblGoodsLimitVector;
        bool insertFromXML(rapidxml::xml_node<>* node, int iOption, tbl_goods& goodsItem, tbl_goods_list& goodsItemList) noexcept;
};