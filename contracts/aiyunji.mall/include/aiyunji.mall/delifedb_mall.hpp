#pragma once
#include <wasm.hpp>
#include <asset.hpp>
#include <table.hpp>

using namespace wasm;

namespace wasm { namespace db {

    #define TABLE_IN_CONTRACT [[wasm::table, wasm::contract("delife")]]

    struct TABLE_IN_CONTRACT credit_ratio_t {
        uint8_t customer;             // 60%
        uint8_t store;                // 10%
        uint8_t store_top;            // 4%
        uint8_t all_customer;         // 10%
        uint8_t platform_top;         // 4%
        uint8_t inviter;              // 5%
        uint8_t agency;               // 2%
        uint8_t citycenter;           // 2%
        uint8_t gas;                  // 3%

        credit_ratio_t(uint8_t c, uint8_t s, uint8_t st, uint8_t ac, uint8_t pt, uint8_t i, uint8_t ag, uint8_t cc, uint8_t g):
                    customer(c), store(s), store_top(st), all_customer(ac), platform_top(pt), inviter(i), agency(ag), citycenter(cc), gas(g) {}
    };

    struct TABLE_IN_CONTRACT mall_config_t {
        credit_ratio_t credit_ratio_conf;

        uint8_t platform_trx_share;         // 30%
        uint8_t dev_trx_share;              // 5%

        regid platform_regid;
        regid devshare_regid;

        uint64_t primary_key()const { return 0ULL; }
        regid scope()const { return DELIFE_SCOPE; }

        mall_config_t() {}

        typedef wasm::table< "mallconf"_n, mall_config_t, uint64_t > table_t;

        WASMLIB_SERIALIZE( mall_config_t, (credit_ratio_conf)(platform_trx_share)(dev_trx_share)(platform_regid)(devshare_regid) )
    };

    struct TABLE_IN_CONTRACT pool_t {
        uint64_t daily_top_share_a;  //10%: platform-wise top 10
        uint64_t daily_top_share_b;  // 4%: platform-wise top 1000

        uint64_t primary_key()const { return 1ULL; }
        uint16_t scope()const { return DELIFE_SCOPE; }

        typedef wasm::table< "pool"_n, pool_t, uint64_t > table_t;
    };

    struct TABLE_IN_CONTRACT store_t {
        uint64_t    id;                 //PK

        uint64_t    daily_top_share;    //  4%: store-wise top 10
        uint64_t    daily_all_share;    // 10%: all store consumers
        uint64_t    created_at;

        store_t(uint64_t i): id(i) {}

        uint64_t primary_key()const { return id; }
        regid scope()const { return DELIFE_SCOPE; }

        typedef wasm::table< "store"_n, store_t, uint64_t > table_t;
    };

    struct TABLE_IN_CONTRACT store_customer_t {
        uint64_t    store_id;
        regid       customer;

        uint64_t    acc_consume_points;

        uint64_t primary_key()const { return customer.value; }
        regid scope()const { return store_id; }

        store_customer_t() {}
        store_customer_t(uint64_t s, regid c): store_id(s), customer(s) {}

        typedef wasm::table< "stcustomer"_n, store_customer_t, uint64_t > table_t;

        WASMLIB_SERIALIZE( store_customer_t,   (store_id)(customer)(acc_consume_points) )
    };

    struct TABLE_IN_CONTRACT customer_t {
        regid       user;                  //PK

        regid       inviter;
        uint64_t    created_at;

        uint64_t primary_key()const { return user.value; }
        uint64_t scope()const { return DELIFE_SCOPE; }

        customer_t(regid u): user(u) {}
        customer_t() {}

        typedef wasm::table< "customer"_n, project_t, uint64_t > table_t;

        WASMLIB_SERIALIZE( customer_t,   (user)(inviter)(created_at) )
    };


} }