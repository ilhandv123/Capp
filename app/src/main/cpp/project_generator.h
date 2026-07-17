#ifndef PROJECT_GENERATOR_H
#define PROJECT_GENERATOR_H

#include <jni.h>
#include <string>
#include <vector>
#include <map>

struct GeneratedFile {
  std::string path;
  std::string content;
};

class ProjectGenerator {
public:
  static std::vector<GeneratedFile> generateProject(
      const std::string& appName,
      const std::string& packageName);

  static std::string getProjectStructure();
  static std::string getVersion();
};

#endif
