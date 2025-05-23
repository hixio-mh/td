//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2021
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/telegram/DialogId.h"
#include "td/telegram/FolderId.h"
#include "td/telegram/td_api.h"

#include "td/utils/common.h"
#include "td/utils/tl_helpers.h"

namespace td {

class Td;

class DialogActionBar {
  int32 distance_ = -1;  // distance to the peer

  bool can_report_spam_ = false;
  bool can_add_contact_ = false;
  bool can_block_user_ = false;
  bool can_share_phone_number_ = false;
  bool can_report_location_ = false;
  bool can_unarchive_ = false;
  bool can_invite_members_ = false;

  friend bool operator==(const unique_ptr<DialogActionBar> &lhs, const unique_ptr<DialogActionBar> &rhs);

 public:
  static unique_ptr<DialogActionBar> create(bool can_report_spam, bool can_add_contact, bool can_block_user,
                                            bool can_share_phone_number, bool can_report_location, bool can_unarchive,
                                            int32 distance, bool can_invite_members);

  bool is_empty() const;

  bool can_report_spam() const {
    return can_report_spam_;
  }

  bool can_unarchive() const {
    return can_unarchive_;
  }

  td_api::object_ptr<td_api::ChatActionBar> get_chat_action_bar_object(DialogType dialog_type,
                                                                       bool hide_unarchive) const;

  void fix(Td *td, DialogId dialog_id, bool is_dialog_blocked, FolderId folder_id);

  bool on_dialog_unarchived();

  bool on_user_contact_added();

  bool on_user_deleted();

  bool on_outgoing_message();

  template <class StorerT>
  void store(StorerT &storer) const {
    bool has_distance = distance_ >= 0;
    BEGIN_STORE_FLAGS();
    STORE_FLAG(can_report_spam_);
    STORE_FLAG(can_add_contact_);
    STORE_FLAG(can_block_user_);
    STORE_FLAG(can_share_phone_number_);
    STORE_FLAG(can_report_location_);
    STORE_FLAG(can_unarchive_);
    STORE_FLAG(can_invite_members_);
    STORE_FLAG(has_distance);
    END_STORE_FLAGS();
    if (has_distance) {
      td::store(distance_, storer);
    }
  }

  template <class ParserT>
  void parse(ParserT &parser) {
    bool has_distance;
    BEGIN_PARSE_FLAGS();
    PARSE_FLAG(can_report_spam_);
    PARSE_FLAG(can_add_contact_);
    PARSE_FLAG(can_block_user_);
    PARSE_FLAG(can_share_phone_number_);
    PARSE_FLAG(can_report_location_);
    PARSE_FLAG(can_unarchive_);
    PARSE_FLAG(can_invite_members_);
    PARSE_FLAG(has_distance);
    END_PARSE_FLAGS();
    if (has_distance) {
      td::parse(distance_, parser);
    }
  }
};

bool operator==(const unique_ptr<DialogActionBar> &lhs, const unique_ptr<DialogActionBar> &rhs);

}  // namespace td
