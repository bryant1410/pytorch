#pragma once

#include <torch/csrc/distributed/rpc/message.h>
#include <torch/csrc/distributed/rpc/rpc_command_base.h>
#include <torch/csrc/distributed/rpc/types.h>
#include <torch/csrc/jit/operator.h>
#include <torch/csrc/jit/pickler.h>
#include <vector>

namespace torch {
namespace distributed {
namespace rpc {

// Temporary solution of RRef operations.
// TODO: Remove all these messages and use rpc + registered functions instead.
class TORCH_API RRefMessageBase : public RpcCommandBase {
 public:
  RRefMessageBase(const RRefId& rrefId, MessageType type)
      : rrefId_(rrefId), type_(type) {}

  virtual ~RRefMessageBase() override = default;

  const RRefId& rrefId();

  virtual Message toMessage() && override;
  static at::IValue fromMessage(const Message& message, MessageType type);

 protected:
  const RRefId rrefId_;
  const MessageType type_;
};

class TORCH_API ForkMessageBase : public RRefMessageBase {
 public:
  ForkMessageBase(const RRefId& rrefId, const ForkId& forkId, MessageType type)
      : RRefMessageBase(rrefId, type), forkId_(forkId) {}

  virtual ~ForkMessageBase() override = default;

  const ForkId& forkId();

  virtual Message toMessage() && override;
  static std::pair<RRefId, ForkId> fromMessage(
      const Message& message,
      MessageType type);

 protected:
  const ForkId forkId_;
};

// UserRRef uses this message to fetch the remote RRef value from the owner.
class TORCH_API ScriptRRefFetchCall final : public RRefMessageBase {
 public:
  explicit ScriptRRefFetchCall(const RRefId& rrefId)
      : RRefMessageBase(rrefId, MessageType::SCRIPT_RREF_FETCH_CALL) {}

  static std::unique_ptr<ScriptRRefFetchCall> fromMessage(
      const Message& message);
};

class TORCH_API PythonRRefFetchCall final : public RRefMessageBase {
 public:
  explicit PythonRRefFetchCall(const RRefId& rrefId)
      : RRefMessageBase(rrefId, MessageType::PYTHON_RREF_FETCH_CALL) {}

  static std::unique_ptr<PythonRRefFetchCall> fromMessage(
      const Message& message);
};

// OwnerRRef uses this message to send the RRef value to a remote UserRRef
class TORCH_API RRefFetchRet final : public RpcCommandBase {
 public:
  explicit RRefFetchRet(std::vector<at::IValue> values)
      : values_(std::move(values)) {}

  const std::vector<at::IValue>& values();

  Message toMessage() && override;
  static std::unique_ptr<RRefFetchRet> fromMessage(const Message& message);

 private:
  std::vector<at::IValue> values_;
};

// UserRRef (regardless it's the creator or not) uses this message to notiify
// OwnerRRef on delete.
class TORCH_API RRefUserDelete final : public ForkMessageBase {
 public:
  RRefUserDelete(const RRefId& rrefId, const ForkId& forkId)
      : ForkMessageBase(rrefId, forkId, MessageType::RREF_USER_DELETE) {}

  static std::unique_ptr<RRefUserDelete> fromMessage(const Message& message);
};

class TORCH_API RemoteRet final : public ForkMessageBase {
 public:
  RemoteRet(const RRefId& rrefId, const ForkId& forkId)
      : ForkMessageBase(rrefId, forkId, MessageType::REMOTE_RET) {}

  static std::unique_ptr<RemoteRet> fromMessage(const Message& message);
};

// A child RRef uses this message to notify its parent that the child has been
// confirmed by the owner.
class TORCH_API RRefChildAccept final : public RpcCommandBase {
 public:
  explicit RRefChildAccept(const ForkId& forkId) : forkId_(forkId) {}

  const ForkId& forkId() const;

  Message toMessage() && override;
  static std::unique_ptr<RRefChildAccept> fromMessage(const Message& message);

 private:
  const ForkId forkId_;
};

// A child RRef uses this message to send a fork request to the owner.
class TORCH_API RRefForkRequest final : public ForkMessageBase {
 public:
  RRefForkRequest(const RRefId& rrefId, const ForkId& forkId)
      : ForkMessageBase(rrefId, forkId, MessageType::RREF_FORK_REQUEST) {}

  static std::unique_ptr<RRefForkRequest> fromMessage(const Message& message);
};

class TORCH_API RRefAck final : public RpcCommandBase {
 public:
  RRefAck() {}

  Message toMessage() && override;
  static std::unique_ptr<RRefAck> fromMessage(const Message& message);
};

} // namespace rpc
} // namespace distributed
} // namespace torch
