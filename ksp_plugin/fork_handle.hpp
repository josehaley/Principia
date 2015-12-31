#pragma once

// Owns a fork.
// TODO(egg): templatize, move to physics, make movable, make NewFork return
// that.
class ForkHandle {
 public:
  ForkHandle(not_null<DiscreteTrajectory<Barycentric>*> trajectory)
      : trajectory_(trajectory) {
    CHECK(!trajectory_->is_root());
  }

  ForkHandle(ForkHandle const&) = delete;
  ForkHandle(ForkHandle&&) = delete;
  ForkHandle& operator=(ForkHandle const&) = delete;
  ForkHandle& operator=(ForkHandle&&) = delete;

  ~ForkHandle() {
    if (trajectory_ != nullptr) {
      trajectory_->parent()->DeleteFork(&trajectory_);
    }
  }

  DiscreteTrajectory<Barycentric>* operator->() {
    return trajectory_;
  }

  DiscreteTrajectory<Barycentric> const* operator->() const {
    return trajectory_;
  }

  DiscreteTrajectory<Barycentric>& operator*() {
    return *trajectory_;
  }

  DiscreteTrajectory<Barycentric> const& operator*() const {
    return *trajectory_;
  }

  not_null<DiscreteTrajectory<Barycentric>*> get() {
    return trajectory_;
  }

  not_null<DiscreteTrajectory<Barycentric> const*> get() const {
    return trajectory_;
  }

 private:
  DiscreteTrajectory<Barycentric>* trajectory_;
};
