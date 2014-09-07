#include "ksp_plugin/plugin.hpp"

namespace principia {
namespace ksp_plugin {

using physics::Body;
using physics::Trajectory;

// Represents a KSP |CelestialBody|.
struct Plugin::Celestial {
  explicit Celestial(GravitationalParameter const& gravitational_parameter) 
    : body(new Body(gravitational_parameter)) {}
  std::unique_ptr<Body const> const body;
  // The parent body for the 2-body approximation. Not owning, should only
  // be null for the sun.
  Celestial const* parent = nullptr;
  // The past and present trajectory of the body.
  std::unique_ptr<Trajectory<World>> history;
};

// Represents a KSP |Vessel|.
struct Plugin::Vessel {
  // Constructs a vessel whose parent is initially |*parent|. |parent| should
  // not be null. No transfer of ownership.
  explicit Vessel(Celestial const* parent) : parent(parent) {
    CHECK(parent != nullptr) << "null parent";
  }
  // A massless |Body|.
  std::unique_ptr<Body const> const body = new Body(GravitationalParameter());
  // The parent body for the 2-body approximation. Not owning, should not be
  // null.
  Celestial const* parent;
  // The past and present trajectory of the body.
  std::unique_ptr<Trajectory<World>> history;
  // Whether to keep the |Vessel| during the next call to |AdvanceTime|.
  bool keep = true;
};

Plugin::Plugin(Date const& initial_time, int const sun_index,
               GravitationalParameter const& sun_gravitational_parameter)
    : current_time_(initial_time) {
  celestials_.insert(
      {sun_index, std::make_unique<Celestial>(sun_gravitational_parameter)});
  sun_->history = std::make_unique<Trajectory<World>>(*sun_);
  sun_->history->Append(current_time_ - Date(),
                        {Displacement<World>(), VelocityOffset<World>()});
}

void Plugin::InsertCelestial(int index,
                             GravitationalParameter gravitational_parameter
                             CelestialRelativeState* state) {
  if (state == nullptr) {
    
  }
  auto const inserted = celestials_.insert(
      {index, std::make_unique<Celestial>(new Body(gravitational_parameter))});
  CHECK(inserted.second) << "Multiple bodies bearing the same index";
}

void Plugin::UpdateCelestialHierarchy(int index, int parent) {
  CHECK(celestials_.find(index) != celestials_.end()) <<
      "No body at index " << index;
  CHECK(celestials_.find(parent) != celestials_.end()) <<
      "No body at index " << parent;
  celestials_[index]->parent = celestials_[parent].get();
}

bool Plugin::InsertOrKeepVessel(std::string guid, int parent) {
  CHECK(celestials_.find(parent) != celestials_.end()) <<
      "No body at index " << parent;
  return vessels_.insert(
      {guid, std::make_unique<Vessel>(celestials_[parent].get())}).second;
}

void Plugin::CleanupVessels() {
  for (auto it = vessels_.cbegin(); it != vessels_.cend();) {
    if(!it->second->keep) {
      // |std::map::erase| invalidates its parameter so we post-increment.
      vessels_.erase(it++);
    } else {
      it->second->keep = false;
      ++it;
    }
  }
}

}  // namespace ksp_plugin
}  // namespace principia
