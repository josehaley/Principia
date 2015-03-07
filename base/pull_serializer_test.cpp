#include "base/pull_serializer.hpp"

#include <vector>

#include "gmock/gmock.h"
#include "serialization/physics.pb.h"

namespace principia {

using serialization::Pair;
using serialization::Point;
using serialization::Quantity;
using serialization::Trajectory;
using ::testing::ElementsAreArray;

namespace base {

class PullSerializerTest : public ::testing::Test {
 protected:
  int const kChunkSize = 99;

  PullSerializerTest()
      : pull_serializer_(std::make_unique<PullSerializer>(kChunkSize)) {
    // Build a biggish protobuf for serialization.
    for (int i = 0; i < 100; ++i) {
      Trajectory::InstantaneousDegreesOfFreedom* idof =
          trajectory_.add_timeline();
      Point* instant = idof->mutable_instant();
      Quantity* scalar = instant->mutable_scalar();
      scalar->set_dimensions(3);
      scalar->set_magnitude(3 * i);
      Pair* dof = idof->mutable_degrees_of_freedom();
      Pair::Element* t1 = dof->mutable_t1();
      Point* point1 = t1->mutable_point();
      Quantity* scalar1 = point1->mutable_scalar();
      scalar1->set_dimensions(1);
      scalar1->set_magnitude(i);
      Pair::Element* t2 = dof->mutable_t2();
      Point* point2 = t2->mutable_point();
      Quantity* scalar2 = point2->mutable_scalar();
      scalar2->set_dimensions(2);
      scalar2->set_magnitude(2 * i);
    }
  }

  std::unique_ptr<PullSerializer> pull_serializer_;
  Trajectory trajectory_;
};

TEST_F(PullSerializerTest, SerializationSizes) {
  pull_serializer_->Start(&trajectory_);
  std::vector<int> actual_sizes;
  std::vector<int> expected_sizes(53, kChunkSize);
  expected_sizes.push_back(53);
  for (;;) {
    Bytes const bytes = pull_serializer_->Pull();
    if (bytes.size == 0) {
      break;
    }
    actual_sizes.push_back(bytes.size);
  }
  EXPECT_THAT(actual_sizes, ElementsAreArray(expected_sizes));
}

TEST_F(PullSerializerTest, SerializationThreading) {
  Trajectory read_trajectory;
  auto serialized_trajectory =
      std::make_unique<std::uint8_t[]>(trajectory_.ByteSize());
  trajectory_.SerializePartialToArray(&serialized_trajectory[0],
                                      trajectory_.ByteSize());

  for (int i = 0; i < 100; ++i) {
    LOG(ERROR)<<"START "<<trajectory_.ByteSize();
    pull_serializer_ =
        std::make_unique<PullSerializer>(kChunkSize);

    auto storage = std::make_unique<std::uint8_t[]>(trajectory_.ByteSize() + 200);
    std::uint8_t* data = &storage[0];
    pull_serializer_->Start(&trajectory_);
    for (;;) {
      Bytes const bytes = pull_serializer_->Pull();
      std::memcpy(data, bytes.data, bytes.size);
      data = &data[bytes.size];
      LOG(ERROR)<<"size "<<data - &storage[0];
      if (bytes.size == 0) {
        break;
      }
    }
    pull_serializer_.reset();

    if (!read_trajectory.ParseFromArray(&storage[0], trajectory_.ByteSize())) {
      for (int i = 0; i < trajectory_.ByteSize(); ++i) {
        if (serialized_trajectory[i] != storage[i]) {
          LOG(FATAL) << "position=" << i
                     << ", expected=" << static_cast<int>(serialized_trajectory[i])
                     << ", actual=" << static_cast<int>(storage[i]);
        }
      }
    }
  }
}

}  // namespace base
}  // namespace principia
