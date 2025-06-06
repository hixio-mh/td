//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2021
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "td/telegram/MessageSender.h"

#include "td/telegram/AuthManager.h"
#include "td/telegram/ContactsManager.h"
#include "td/telegram/MessagesManager.h"
#include "td/telegram/Td.h"

#include "td/utils/algorithm.h"
#include "td/utils/misc.h"

namespace td {

td_api::object_ptr<td_api::MessageSender> get_message_sender_object_const(Td *td, UserId user_id, DialogId dialog_id,
                                                                          const char *source) {
  if (dialog_id.is_valid() && td->messages_manager_->have_dialog(dialog_id)) {
    return td_api::make_object<td_api::messageSenderChat>(dialog_id.get());
  }
  if (!user_id.is_valid()) {
    // can happen only if the server sends a message with wrong sender
    LOG(ERROR) << "Receive message with wrong sender " << user_id << '/' << dialog_id << " from " << source;
    user_id = td->contacts_manager_->add_service_notifications_user();
  }
  return td_api::make_object<td_api::messageSenderUser>(td->contacts_manager_->get_user_id_object(user_id, source));
}

td_api::object_ptr<td_api::MessageSender> get_message_sender_object_const(Td *td, DialogId dialog_id,
                                                                          const char *source) {
  if (dialog_id.get_type() == DialogType::User) {
    return get_message_sender_object_const(td, dialog_id.get_user_id(), DialogId(), source);
  }
  return get_message_sender_object_const(td, UserId(), dialog_id, source);
}

td_api::object_ptr<td_api::MessageSender> get_message_sender_object(Td *td, UserId user_id, DialogId dialog_id,
                                                                    const char *source) {
  if (dialog_id.is_valid() && !td->messages_manager_->have_dialog(dialog_id)) {
    LOG(ERROR) << "Failed to find " << dialog_id;
    td->messages_manager_->force_create_dialog(dialog_id, source);
  }
  if (!user_id.is_valid() && td->auth_manager_->is_bot()) {
    td->contacts_manager_->add_anonymous_bot_user();
    td->contacts_manager_->add_service_notifications_user();
  }
  return get_message_sender_object_const(td, user_id, dialog_id, source);
}

td_api::object_ptr<td_api::MessageSender> get_message_sender_object(Td *td, DialogId dialog_id, const char *source) {
  if (dialog_id.get_type() == DialogType::User) {
    return get_message_sender_object(td, dialog_id.get_user_id(), DialogId(), source);
  }
  return get_message_sender_object(td, UserId(), dialog_id, source);
}

vector<DialogId> get_message_sender_dialog_ids(Td *td,
                                               const vector<telegram_api::object_ptr<telegram_api::Peer>> &peers) {
  vector<DialogId> message_sender_dialog_ids;
  message_sender_dialog_ids.reserve(peers.size());
  for (auto &peer : peers) {
    DialogId dialog_id(peer);
    if (!dialog_id.is_valid()) {
      LOG(ERROR) << "Receive invalid " << dialog_id << " as message sender";
      continue;
    }
    if (dialog_id.get_type() == DialogType::User) {
      if (!td->contacts_manager_->have_user(dialog_id.get_user_id())) {
        LOG(ERROR) << "Have no info about " << dialog_id.get_user_id();
        continue;
      }
    } else {
      if (!td->messages_manager_->have_dialog_info(dialog_id)) {
        continue;
      }
      td->messages_manager_->force_create_dialog(dialog_id, "get_message_sender_dialog_ids");
      if (!td->messages_manager_->have_dialog(dialog_id)) {
        continue;
      }
    }
    message_sender_dialog_ids.push_back(dialog_id);
  }
  return message_sender_dialog_ids;
}

td_api::object_ptr<td_api::messageSenders> convert_message_senders_object(
    Td *td, const vector<telegram_api::object_ptr<telegram_api::Peer>> &peers) {
  auto dialog_ids = get_message_sender_dialog_ids(td, peers);
  auto message_senders = transform(dialog_ids, [td](DialogId dialog_id) {
    return get_message_sender_object(td, dialog_id, "convert_message_senders_object");
  });
  return td_api::make_object<td_api::messageSenders>(narrow_cast<int32>(dialog_ids.size()), std::move(message_senders));
}

Result<DialogId> get_message_sender_dialog_id(Td *td,
                                              const td_api::object_ptr<td_api::MessageSender> &message_sender_id,
                                              bool check_access, bool allow_empty) {
  if (message_sender_id == nullptr) {
    if (allow_empty) {
      return DialogId();
    }
    return Status::Error(400, "Message sender must be non-empty");
  }
  switch (message_sender_id->get_id()) {
    case td_api::messageSenderUser::ID: {
      auto user_id = UserId(static_cast<const td_api::messageSenderUser *>(message_sender_id.get())->user_id_);
      if (!user_id.is_valid()) {
        if (allow_empty && user_id == UserId()) {
          return DialogId();
        }
        return Status::Error(400, "Invalid user identifier specified");
      }
      if (check_access && !td->contacts_manager_->have_user_force(user_id)) {
        return Status::Error(400, "Unknown user identifier specified");
      }
      return DialogId(user_id);
    }
    case td_api::messageSenderChat::ID: {
      auto dialog_id = DialogId(static_cast<const td_api::messageSenderChat *>(message_sender_id.get())->chat_id_);
      if (!dialog_id.is_valid()) {
        if (allow_empty && dialog_id == DialogId()) {
          return DialogId();
        }
        return Status::Error(400, "Invalid chat identifier specified");
      }
      if (check_access) {
        bool is_user = dialog_id.get_type() == DialogType::User;
        if (is_user ? !td->contacts_manager_->have_user_force(dialog_id.get_user_id())
                    : !td->messages_manager_->have_dialog_force(dialog_id, "get_message_sender_dialog_id")) {
          return Status::Error(400, "Unknown chat identifier specified");
        }
      }
      return dialog_id;
    }
    default:
      UNREACHABLE();
      return DialogId();
  }
}

}  // namespace td
