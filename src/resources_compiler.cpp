#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr std::string_view kHelpText(R"%(
usage:
resources_compiler --sources=file1,file2 --output=full/path/to/file_without_extension
without supplying --sources param, an empty resources holder classes will be generated
once again, output file path with file name only - no extension please, because tool will use it as base for .h and .cpp files
note: spaces are not allowed in paths nor between the tags nor the equal signs!
)%");

constexpr std::string_view kHeaderTop(R"%(#pragma once

/*)%");

constexpr std::string_view kHeaderBottom(R"%(
*/

#include <string>
#include <string_view>
#include <unordered_map>

namespace resources {

class manager final {
 public:
  manager();

  std::string_view get(const std::string_view& name) const;

 private:
  std::unordered_map<std::string_view, std::string_view> resources_;
};

}  // namespace resources
)%");

constexpr std::string_view kSourceTop(R"%(
*/

namespace resources {
namespace {
// resources list

)%");

constexpr std::string_view kSourceMiddle(R"%(// resources list

template <typename T, size_t size>
size_t array_size(T (&)[size]) {
  return size;
}

}  // namespace

manager::manager() {
  // resources list

)%");

constexpr std::string_view kSourceBottom(R"%(
  // resources list
}

std::string_view manager::get(const std::string_view& name) const {
  const auto iterator = resources_.find(name);
  if (iterator == resources_.end()) {
    return {};
  }
  return iterator->second;
}

}  // namespace resources
)%");

constexpr std::string_view kBanner(R"%(

      .:+oooooooooooooooooooooooooooooooooooooo: `/ooooooooooo/` :ooooo+/-`
   `+d##########################################sh#############do#########Ns.
  :#####N#ddddddddddddddddddddddddddddddN######h.:hdddddddddddh/.ydddd######N+
 :N###N+.        .-----------.`       `+#####d/   .-----------.        `:#####/
 h####/         :############Nd.    `/d#####+`   sN###########Ny         -#####
 h####/         :#N##########Nd.   :h####No`     oN###########Ny         -#####
 :N###No.`       `-----------.`  -yN###Ns.       `.-----------.`       `/#####/
  :#####N#####d/.yd##########do.sN#####################################N####N+
   `+d#########do#############N+###########################################s.
      .:+ooooo/` :+oooooooooo+. .+ooooooooooooooooooooooooooooooooooooo+/.

        C E Z E O  S O F T W A R E    R E S O U R C E S  C O M P I L E R
)%");

constexpr std::string_view kSourcesListTag("--sources=");
constexpr std::string_view kOutputPathTag("--output=");

constexpr const char* kReset = "\033[0m";          /* Reset output */
constexpr const char* kYellow = "\033[1m\033[33m"; /* Bright Yellow */
constexpr const char* kCyan = "\033[1m\033[36m";   /* Bright Cyan */

// slice string with one delimiter char
// zero length pieces will be skipped
template <typename container_type>
void each_slice_view(const char* begin, size_t size, char delimiter, container_type& container) {
  const char* first = begin;
  const char* current = begin;
  const char* end = begin + size;
  for (; begin != end; ++begin) {
    if (*begin == delimiter) {
      if (first != current) {
        container.push_back(std::basic_string_view<char>(first, size_t(current - first)));
      }
      first = ++current;
    } else {
      ++current;
    }
  }
  if (first != current) {
    container.push_back(std::basic_string_view<char>(first, size_t(current - first)));
  }
}

// if input container begins with match
template <typename container_type>
bool begins_with(const container_type& input, const container_type& match) {
  return input.size() >= match.size() && std::equal(match.begin(), match.end(), input.begin());
}

struct file_entry final {
  file_entry(std::string source_array, std::string source_map)
      : source_array_entry(std::move(source_array)), source_map_entry(std::move(source_map)) {}
  std::string source_array_entry;
  std::string source_map_entry;
};

// save the string to file
void save_string_to_file(const std::string& data, const fs::path& file_path) {
  std::ofstream header_file_output(file_path, std::ios::out | std::ios::binary);
  if (header_file_output.is_open()) {
    header_file_output.write(data.data(), data.length());
  } else {
    SPDLOG_ERROR("can't open file for write operation: {}", file_path.u8string());
  }
  header_file_output.close();
  SPDLOG_INFO("file generated: '{}', file size: {}", file_path.u8string(), data.length());
}

}  // namespace

void print_help() {
  spdlog::set_pattern("%v");
  SPDLOG_INFO("{}{}{}", kYellow, kBanner, kReset);
  SPDLOG_INFO(kHelpText);
}

int main(int argc, char* argv[]) {
  spdlog::set_pattern("[%H:%M:%S.%e] %^[%l]%$ %v");
  if (argc < 2) {
    SPDLOG_ERROR("not enough params");
    print_help();
    return -1;
  }

  std::vector<std::string_view> source_files;
  std::string_view output_file;
  for (int i = 1; i < argc; i++) {
    const std::string_view argument(argv[i]);
    if (begins_with(argument, kSourcesListTag)) {
      // --sources=
      const auto sources_paths = argument.substr(kSourcesListTag.size());
      each_slice_view(sources_paths.data(), sources_paths.size(), ',', source_files);
    } else if (begins_with(argument, kOutputPathTag)) {
      // --output=
      output_file = argument.substr(kOutputPathTag.size());
    } else {
      SPDLOG_WARN("unknown option: {}", argument);
    }
  }

  if (source_files.empty()) {
    SPDLOG_WARN("no source files specified, an empty project will be generated");
  }

  if (nullptr == output_file.data() || output_file.size() == 0) {
    SPDLOG_ERROR("no output file specified");
    print_help();
    return -3;
  }

  // filesystem throw errors if no std::errc supplied
  try {
    // process output files
    fs::path output_header_path(output_file);
    output_header_path.replace_extension(".h");
    fs::path output_source_path(output_file);
    output_source_path.replace_extension(".cpp");

    if (fs::exists(output_header_path)) {
      // delete output header file if it's exist
      SPDLOG_INFO("output header file exists: we are going to delete it: {}", output_header_path.u8string());
      fs::remove(output_header_path);
    }

    if (fs::exists(output_source_path)) {
      // delete output source file if it's exist
      SPDLOG_INFO("output source file exists: we are going to delete it: {}", output_source_path.u8string());
      fs::remove(output_source_path);
    }

    // container for resource files
    std::vector<file_entry> result_data;
    size_t result_source_size = kBanner.size();

    // process files
    for (const auto& source_file : source_files) {
      const fs::path file_path(source_file);
      // open the stream to 'lock' the file.
      std::ifstream file_stream(file_path, std::ios::in | std::ios::binary);
      const auto file_size = fs::file_size(file_path);

      if (file_size > 0) {
        if (file_size > (1024 * 1024 * 16)) {
          SPDLOG_WARN("we are going to add file: '{}', with size: {} bytes to the resources!", file_size, source_file);
        }

        std::vector<char> binary_buffer;
        binary_buffer.resize(file_size);

        // read the whole file into the buffer
        file_stream.read(binary_buffer.data(), binary_buffer.size());
        // 4 chars for byte, '255,' + new line for every 0x1f chars + 64 for { }
        // etc.
        const size_t precalc_stirng_size((binary_buffer.size() * 4) + (binary_buffer.size() / 0x1F) + 64);

        // variable safe name for file
        std::string resource_name(file_path.filename().u8string());
        std::replace(resource_name.begin(), resource_name.end(), '.', '_');

        // build header entry
        std::string source_array_entry;
        source_array_entry.reserve(precalc_stirng_size);
        source_array_entry.append(fmt::format("constexpr const char {}[{}] = ", resource_name, binary_buffer.size()));
        source_array_entry.append("{");

        size_t char_counter = 0;
        // use unsigned type in for
        for (unsigned char byte : binary_buffer) {
          if ((char_counter & 0x1F) == 0) {
            // add some line breaks
            source_array_entry.append("\n");
          }
          source_array_entry.append(fmt::format("{},", byte));
          ++char_counter;
        }

        const std::string_view end_token("};\n\n");
        const std::string_view token_at_end(",");
        const auto last_comma = source_array_entry.rfind(token_at_end);
        if (std::string::npos != last_comma) {
          source_array_entry.replace(last_comma, last_comma + token_at_end.size(), end_token);
        } else {
          SPDLOG_WARN("end token is not found, so it looks like an error");
          source_array_entry.append(end_token);
        }

        // now build the map insertion
        std::string source_map_entry(
            fmt::format("  resources_.insert({{\"{}\", std::string_view({}, array_size({}))}});\n",
                        resource_name,
                        resource_name,
                        resource_name));
        double increase = (double)source_array_entry.size() / (double)file_size;
        SPDLOG_INFO("file: '{}' added, size in binary: {}, size in source: {} bytes, increase: {:.2f}x",
                    source_file,
                    file_size,
                    source_array_entry.size(),
                    increase);
        // store size calculations before move
        result_source_size += source_array_entry.size() + source_map_entry.size();
        // move to result_data
        result_data.emplace_back(std::move(source_array_entry), std::move(source_map_entry));
      } else {
        SPDLOG_WARN("empty file skipped: {}", source_file);
      }
      file_stream.close();
    }

    // now build the result header
    const auto precalc_header_size(kBanner.size() + kHeaderTop.size() + kHeaderBottom.size() + 256);

    const auto precalc_source_size(result_source_size + kSourceTop.size() + kSourceMiddle.size() +
                                   kSourceBottom.size() + 256);
    //
    // build header
    //
    std::string result_header;
    result_header.reserve(precalc_header_size);
    result_header.append(kHeaderTop);
    result_header.append(kBanner);
    result_header.append(kHeaderBottom);
    save_string_to_file(result_header, output_header_path);

    //
    // build source
    //
    std::string result_source;
    result_source.reserve(precalc_source_size);
    result_source.append(fmt::format("#include \"{}\"\n\n/*", output_header_path.filename().u8string()));
    result_source.append(kBanner);
    result_source.append(kSourceTop);

    for (auto& result_entry : result_data) {
      // add arrays with actual data
      result_source.append(std::move(result_entry.source_array_entry));
    }
    result_source.append(kSourceMiddle);

    for (auto& result_entry : result_data) {
      // add map inserts
      result_source.append(std::move(result_entry.source_map_entry));
    }
    result_source.append(kSourceBottom);

    // save the result
    save_string_to_file(result_source, output_source_path);

  } catch (const std::exception& e) {
    SPDLOG_ERROR("exception during processing: {}", e.what());
    return -255;
  }

  return 0;
}
