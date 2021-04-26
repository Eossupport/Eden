#pragma once

#include <constants.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/map_macro.h>
#include <globals.hpp>
#include <string>
#include <utils.hpp>

#define EDEN_FORWARD_FUNCTION(var, fun) auto fun() const { return std::visit([](auto& value){ return value.fun(); }, var); }
#define EDEN_FORWARD_FUNCTIONS(var, ...) EOSIO_MAP_REUSE_ARG0(EDEN_FORWARD_FUNCTION, var, __VA_ARGS__)

#define EDEN_FORWARD_MEMBER(var, member)                                \
   decltype(auto) member() { return std::visit([](auto& value) -> decltype(auto) { return (value.member); }, var); } \
   decltype(auto) member() const { return std::visit([](auto& value) -> decltype(auto) { return (value.member); }, var); }
#define EDEN_FORWARD_MEMBERS(var, ...) EOSIO_MAP_REUSE_ARG0(EDEN_FORWARD_MEMBER, var, __VA_ARGS__)

namespace eden
{
   struct new_member_profile
   {
      std::string name;
      std::string img;
      std::string bio;
      std::string social;
   };
   EOSIO_REFLECT(new_member_profile, name, img, bio, social)

   struct induction_v0
   {
      uint64_t id;
      eosio::name inviter;
      eosio::name invitee;
      uint32_t endorsements;
      eosio::block_timestamp created_at;
      std::string video;
      new_member_profile new_member_profile;

      uint64_t primary_key() const { return id; }
      uint128_t get_invitee_inviter() const { return combine_names(invitee, inviter); }
      uint128_t get_inviter_invitee() const { return combine_names(inviter, invitee); }
      uint64_t get_created_key() const { return uint64_t{created_at.slot}; }
   };
   EOSIO_REFLECT(induction_v0,
                 id,
                 inviter,
                 invitee,
                 endorsements,
                 created_at,
                 video,
                 new_member_profile)

   struct induction {
      induction() = default;
      induction(const induction&) = delete;
      std::variant<induction_v0> value;
      EDEN_FORWARD_MEMBERS(value, id, inviter, invitee, endorsements, created_at, video, new_member_profile)
      EDEN_FORWARD_FUNCTIONS(value, primary_key, get_invitee_inviter, get_inviter_invitee, get_created_key)
   };
   EOSIO_REFLECT(induction, value)

   using induction_table_type = eosio::multi_index<
       "induction"_n,
       induction,
       eosio::indexed_by<
           "byinvitee"_n,
           eosio::const_mem_fun<induction, uint128_t, &induction::get_invitee_inviter>>,
       eosio::indexed_by<
           "byinviter"_n,
           eosio::const_mem_fun<induction, uint128_t, &induction::get_inviter_invitee>>,
       eosio::indexed_by<"bycreated"_n,
                         eosio::const_mem_fun<induction, uint64_t, &induction::get_created_key>>>;

   struct endorsement
   {
      uint64_t id;
      eosio::name inviter;
      eosio::name invitee;
      eosio::name endorser;
      uint64_t induction_id;
      bool endorsed;

      uint64_t primary_key() const { return id; }
      uint128_t get_endorser_key() const { return uint128_t{endorser.value} << 64 | induction_id; }
      uint64_t induction_id_key() const { return induction_id; }
   };
   EOSIO_REFLECT(endorsement, id, inviter, invitee, endorser, induction_id, endorsed)

   using endorsement_table_type = eosio::multi_index<
       "endorsement"_n,
       endorsement,
       eosio::indexed_by<
           "byendorser"_n,
           eosio::const_mem_fun<endorsement, uint128_t, &endorsement::get_endorser_key>>,
       eosio::indexed_by<
           "byinduction"_n,
           eosio::const_mem_fun<endorsement, uint64_t, &endorsement::induction_id_key>>>;

   // This table is temporary.  It is used to forward information required by the
   // NFT creation notifications.  Rows should always be deleted in the same
   // transaction in which they are created.
   struct endorsed_induction
   {
      eosio::name invitee;
      uint64_t induction_id;
      uint64_t primary_key() const { return invitee.value; }
   };
   EOSIO_REFLECT(endorsed_induction, invitee, induction_id);
   using endorsed_induction_table_type = eosio::multi_index<"endind"_n, endorsed_induction>;

   class inductions
   {
     private:
      eosio::name contract;
      induction_table_type induction_tb;
      endorsement_table_type endorsement_tb;
      globals globals;

      void check_new_induction(eosio::name invitee, eosio::name inviter) const;
      void check_valid_induction(const induction& induction) const;
      void validate_profile(const new_member_profile& new_member_profile) const;
      void validate_video(const std::string& video) const;
      void check_valid_endorsers(eosio::name inviter,
                                 const std::vector<eosio::name>& witnesses) const;
      void reset_endorsements(uint64_t induction_id);
      void maybe_create_nft(const induction& induction_id);

     public:
      inductions(eosio::name contract)
          : contract(contract),
            induction_tb(contract, default_scope),
            endorsement_tb(contract, default_scope),
            globals(contract)
      {
      }

      const induction& get_induction(uint64_t id) const;
      const induction& get_endorsed_induction(eosio::name invitee) const;

      void initialize_induction(uint64_t id,
                                eosio::name inviter,
                                eosio::name invitee,
                                const std::vector<eosio::name>& witnesses);

      void update_profile(const induction& induction, const new_member_profile& new_member_profile);

      void update_video(const induction& induction, const std::string& video);

      void endorse(const induction& induction,
                   eosio::name account,
                   eosio::checksum256 induction_data_hash);

      bool is_endorser(uint64_t id, eosio::name witness) const;

      void create_nfts(const induction& induction, int32_t template_id);
      void start_auction(const induction& induction, uint64_t asset_id);
      void erase_induction(const induction& induction);
      void create_induction(uint64_t id,
                            eosio::name inviter,
                            eosio::name invitee,
                            uint32_t endorsements,
                            const std::string& video = {});

      void create_endorsement(eosio::name inviter,
                              eosio::name invitee,
                              eosio::name endorser,
                              uint64_t induction_id);

      // Should only be used during genesis
      void endorse_all(const induction& induction);

      // this method is used only for administrative purposes,
      // it should never be used outside genesis or test environments
      void clear_all();
   };

}  // namespace eden
