# Automatically generated by scripts/boost/generate-ports.ps1

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO boostorg/outcome
    REF boost-1.79.0
    SHA512 e0e4587ea19e70047790e86496d85a7c1e310e8b33af99dfbcad330243b9ff390d710cfebda15a94de4497f4a85498c051c043adbe8c41770b747a742a34634c
    HEAD_REF master
)

include(${CURRENT_INSTALLED_DIR}/share/boost-vcpkg-helpers/boost-modular-headers.cmake)
boost_modular_headers(SOURCE_PATH ${SOURCE_PATH})