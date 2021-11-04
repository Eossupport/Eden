#pragma once

#define EOSIO_ABIGEN_ITEMauth_actions(ns, variant_name, missing_struct_name)        \
   ([&] {                                                                           \
      gen.def.structs.push_back(eosio::struct_def{missing_struct_name});            \
      eosio::variant_def vdef{variant_name};                                        \
      ns::for_each_auth_action([&](uint32_t index, const char* name, const auto&) { \
         if (index >= vdef.types.size())                                            \
            vdef.types.resize(index + 1, missing_struct_name);                      \
         vdef.types[index] = name;                                                  \
      });                                                                           \
      gen.def.variants.value.push_back(std::move(vdef));                            \
   })();                                                                            \
   , 1
