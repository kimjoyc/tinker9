#ifndef TINKER_TEST_OS_H_
#define TINKER_TEST_OS_H_

#include "util/macro.h"
#include <string>

TINKER_NAMESPACE_BEGIN
namespace test {
/**
 * @brief
 * Write a file in the current working directory in its constructor and remove
 * this file in its destructor.
 */
class file {
private:
  bool good_;
  std::string name_;

public:
  file(const std::string& name, const std::string& content);
  ~file();
};

void test_begin_1_xyz(int argc, const char** argv);
void test_end();
}
TINKER_NAMESPACE_END

#endif
