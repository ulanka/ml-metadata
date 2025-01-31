/* Copyright 2019 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include <memory>

#include "gflags/gflags.h"

#include "absl/memory/memory.h"
#include "ml_metadata/metadata_store/mysql_metadata_source.h"
#include "ml_metadata/metadata_store/test_mysql_metadata_source_initializer.h"
#include "ml_metadata/proto/metadata_store.pb.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/core/status_test_util.h"

DEFINE_string(db_name, "", "Name of MySQL database to connect to");
DEFINE_string(user_name, "", "MYSQL login id");
DEFINE_string(password, "",
              "Password for user_name. If empty, only MYSQL user ids that "
              "don't have a password set are allowed to connect.");
DEFINE_string(host_name, "", "Host name or IP address of the MYSQL server.");
DEFINE_int32(port, 3306,
             "TCP port number that the MYSQL server accepts connection on. If "
             "not set, the default MYSQL port (3306) is used.");
DEFINE_string(socket, "", "Unix socket file for connecting to MYSQL server.");

namespace ml_metadata {
namespace testing {

// Provides a TestMySqlMetadataSourcesInitializer implementation that
// instantiates a MySqlMetadataSource backed by a MYSQL server as specified by
// the flags listed above.
class TestStandaloneMySqlMetadataSourceInitializer
    : public TestMySqlMetadataSourceInitializer {
 public:
  TestStandaloneMySqlMetadataSourceInitializer() = default;
  ~TestStandaloneMySqlMetadataSourceInitializer() override = default;

  MySqlMetadataSource* Init(ConnectionType connection_type) override {
    MySQLDatabaseConfig config;
    config.set_database((FLAGS_db_name));
    config.set_user((FLAGS_user_name));
    config.set_password((FLAGS_password));
    switch (connection_type) {
      case ConnectionType::kTcp:
        config.set_port((FLAGS_port));
        config.set_host((FLAGS_host_name));
        break;
      case ConnectionType::kSocket:
        config.set_socket((FLAGS_socket));
        break;
      default:
        QCHECK(false) << "Invalid connection_type: "
                      << static_cast<int>(connection_type);
    }

    metadata_source_ = absl::make_unique<MySqlMetadataSource>(config);
    TF_CHECK_OK(metadata_source_->Connect());
    TF_CHECK_OK(metadata_source_->Begin());
    TF_CHECK_OK(metadata_source_->ExecuteQuery(
        "DROP DATABASE IF EXISTS " + config.database(), nullptr));
    TF_CHECK_OK(metadata_source_->Commit());
    TF_CHECK_OK(metadata_source_->Close());
    return metadata_source_.get();
  }

  void Cleanup() override { metadata_source_ = nullptr; }

 private:
  std::unique_ptr<MySqlMetadataSource> metadata_source_;
};

std::unique_ptr<TestMySqlMetadataSourceInitializer>
GetTestMySqlMetadataSourceInitializer() {
  return absl::make_unique<TestStandaloneMySqlMetadataSourceInitializer>();
}

}  // namespace testing
}  // namespace ml_metadata
