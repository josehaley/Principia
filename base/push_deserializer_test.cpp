#include "base/push_deserializer.hpp"

#include <list>
#include <string>
#include <thread>
#include <vector>

#include "base/bytes.hpp"
#include "base/not_null.hpp"
#include "base/pull_serializer.hpp"
#include "gmock/gmock.h"
#include "serialization/physics.pb.h"

namespace principia {

using serialization::Pair;
using serialization::Point;
using serialization::Quantity;
using serialization::Trajectory;
using ::testing::ElementsAreArray;

namespace base {

namespace {
const char kStart[] = "START";
}  // namespace

class PushDeserializerTest : public ::testing::Test {
 protected:
  int const kDeserializerChunkSize = 99;
  int const kSerializerChunkSize = 99;
  int const kNumberOfChunks = 3;

  PushDeserializerTest()
      : pull_serializer_(
            std::make_unique<PullSerializer>(kSerializerChunkSize)),
        push_deserializer_(std::make_unique<PushDeserializer>(
            kDeserializerChunkSize, kNumberOfChunks)),
        stream_(std::bind(&PushDeserializerTest::OnEmpty, this, &strings_)) {
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

  // Returns the first string in the list.  Note that the very first string is
  // always discarded.
  Bytes OnEmpty(not_null<std::list<std::string>*> const strings) {
    strings->pop_front();
    CHECK(!strings->empty());
    std::string& front = strings->front();
    return Bytes(reinterpret_cast<std::uint8_t const*>(front.c_str()),
                 front.size());
  }

  //TODO(phl):not_null
  std::unique_ptr<PullSerializer> pull_serializer_;
  std::unique_ptr<PushDeserializer> push_deserializer_;
  Trajectory trajectory_;
  internal::DelegatingArrayInputStream stream_;
  std::list<std::string> strings_;
};

TEST_F(PushDeserializerTest, Stream) {
  void const* data;
  int size;

  strings_ = {kStart, "abc"};
  EXPECT_TRUE(stream_.Next(&data, &size));
  EXPECT_EQ(3, size);
  EXPECT_EQ("abc", std::string(static_cast<char const*>(data), size));
  EXPECT_EQ(3, stream_.ByteCount());

  strings_ = {kStart, ""};
  EXPECT_FALSE(stream_.Next(&data, &size));
  EXPECT_EQ(3, stream_.ByteCount());

  strings_ = {kStart, "abc", "xyzt"};
  EXPECT_TRUE(stream_.Next(&data, &size));
  EXPECT_EQ(3, size);
  EXPECT_EQ("abc", std::string(static_cast<char const*>(data), size));
  EXPECT_TRUE(stream_.Next(&data, &size));
  EXPECT_EQ(4, size);
  EXPECT_EQ("xyzt", std::string(static_cast<char const*>(data), size));
  EXPECT_EQ(10, stream_.ByteCount());

  strings_ = {kStart, "abc", "xyzt", "uvw", ""};
  EXPECT_TRUE(stream_.Next(&data, &size));
  EXPECT_EQ(3, size);
  EXPECT_TRUE(stream_.Skip(2));
  EXPECT_TRUE(stream_.Next(&data, &size));
  EXPECT_EQ(2, size);
  EXPECT_EQ("zt", std::string(static_cast<char const*>(data), size));
  EXPECT_FALSE(stream_.Skip(5));
  EXPECT_EQ(20, stream_.ByteCount());

  strings_ = {kStart, "abc"};
  EXPECT_TRUE(stream_.Next(&data, &size));
  EXPECT_EQ(3, size);
  stream_.BackUp(1);
  EXPECT_TRUE(stream_.Next(&data, &size));
  EXPECT_EQ(1, size);
  EXPECT_EQ("c", std::string(static_cast<char const*>(data), size));
  EXPECT_EQ(23, stream_.ByteCount());
}

TEST_F(PushDeserializerTest, Deserialization) {
  for (int i = 0; i < 100; ++i) {
    LOG(ERROR)<<"START "<<trajectory_.ByteSize();
    pull_serializer_ =
        std::make_unique<PullSerializer>(kSerializerChunkSize);
    push_deserializer_ = std::make_unique<PushDeserializer>(
        kDeserializerChunkSize, kNumberOfChunks);

    auto storage = std::make_unique<std::uint8_t[]>(trajectory_.ByteSize() + 200);
    std::uint8_t* data = &storage[0];
    Trajectory read_trajectory;
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

    int sz = data - &storage[0];
    CHECK(read_trajectory.ParseFromArray(&storage[0], sz));

    push_deserializer_->Start(&read_trajectory);
    data = &storage[0];
    push_deserializer_->Push(Bytes(data, sz));
    push_deserializer_->Push(Bytes());

    // Destroying the deserializer waits until deserialization is done.  It is
    // important that this happens before |storage| is destroyed.
    push_deserializer_.reset();
  }
}

//TEST_F(PushDeserializerTest, Deserialization) {
//  for (int i = 0; i < 100; ++i) {
//    LOG(ERROR)<<"START "<<trajectory_.ByteSize();
//    pull_serializer_ =
//        std::make_unique<PullSerializer>(kSerializerChunkSize);
//    push_deserializer_ = std::make_unique<PushDeserializer>(
//        kDeserializerChunkSize, kNumberOfChunks);
//
//    auto storage = std::make_unique<std::uint8_t[]>(trajectory_.ByteSize() + 200);
//    std::uint8_t* data = &storage[0];
//    Trajectory read_trajectory;
//    pull_serializer_->Start(&trajectory_);
//    push_deserializer_->Start(&read_trajectory);
//    for (;;) {
//      Bytes const bytes = pull_serializer_->Pull();
//      std::memcpy(data, bytes.data, bytes.size);
//      push_deserializer_->Push(Bytes(data, bytes.size));
//      data = &data[bytes.size];
//      LOG(ERROR)<<"size "<<data - &storage[0];
//      if (bytes.size == 0) {
//        break;
//      }
//    }
//
//    // Destroying the deserializer waits until deserialization is done.  It is
//    // important that this happens before |storage| is destroyed.
//    pull_serializer_.reset();
//    push_deserializer_.reset();
//  }
//}

}  // namespace base
}  // namespace principia
