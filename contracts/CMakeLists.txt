cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

set(ICON_BASE_URL "http://127.0.0.1/ricardian_assets/mgp.contracts/icons")

# add_subdirectory(aiyunji.coin)
# add_subdirectory(aiyunji.love)
# add_subdirectory(aiyunji.mall)
# add_subdirectory(aiyunji.mall.lite)
add_subdirectory(aiyunji.swap)