#include "framework/assets/ThemeManifest.hpp"

#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace pumpdx::assets {

namespace {

class JsonReader final {
public:
    explicit JsonReader(std::string_view source)
        : source_(source) {
    }

    [[nodiscard]] std::unordered_map<std::string, std::string> ReadRootObject() {
        return ReadStringObject();
    }

    void RequireEnd() {
        SkipWhitespace();
        if (position_ != source_.size()) {
            throw std::runtime_error("Unexpected trailing content in theme manifest.");
        }
    }

private:
    [[nodiscard]] std::unordered_map<std::string, std::string> ReadStringObject() {
        Expect('{');
        std::unordered_map<std::string, std::string> values;
        SkipWhitespace();
        if (TryConsume('}')) {
            return values;
        }

        while (true) {
            const auto key = ReadString();
            Expect(':');
            const auto value = ReadString();
            if (!values.emplace(key, value).second) {
                throw std::runtime_error("Duplicate key in theme manifest.");
            }

            SkipWhitespace();
            if (TryConsume('}')) {
                return values;
            }
            Expect(',');
        }
    }

    [[nodiscard]] std::string ReadString() {
        Expect('"');
        std::string value;
        while (position_ < source_.size()) {
            const auto character = source_[position_++];
            if (character == '"') {
                return value;
            }
            if (character == '\\') {
                if (position_ >= source_.size()) {
                    throw std::runtime_error("Incomplete escape sequence in theme manifest.");
                }
                const auto escaped = source_[position_++];
                switch (escaped) {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default: throw std::runtime_error("Unsupported escape sequence in theme manifest.");
                }
                continue;
            }
            value.push_back(character);
        }

        throw std::runtime_error("Unterminated string in theme manifest.");
    }

    void Expect(const char expected) {
        SkipWhitespace();
        if (position_ >= source_.size() || source_[position_] != expected) {
            throw std::runtime_error("Malformed theme manifest.");
        }
        ++position_;
    }

    [[nodiscard]] bool TryConsume(const char expected) {
        SkipWhitespace();
        if (position_ >= source_.size() || source_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
    }

    void SkipWhitespace() {
        while (position_ < source_.size()
            && std::isspace(static_cast<unsigned char>(source_[position_])) != 0) {
            ++position_;
        }
    }

    std::string_view source_;
    std::size_t position_ = 0;
};

[[nodiscard]] const std::string& RequireValue(
    const std::unordered_map<std::string, std::string>& values,
    const std::string_view key) {
    const auto found = values.find(std::string(key));
    if (found == values.end() || found->second.empty()) {
        throw std::runtime_error("Theme manifest is missing a required value.");
    }
    return found->second;
}

[[nodiscard]] std::array<float, 4> ParseColor(const std::string_view encodedColor) {
    if (encodedColor.size() != 7 && encodedColor.size() != 9) {
        throw std::runtime_error("Theme colors must use #RRGGBB or #RRGGBBAA.");
    }
    if (encodedColor.front() != '#') {
        throw std::runtime_error("Theme colors must begin with '#'.");
    }

    const auto nibble = [](const char character) -> unsigned int {
        if (character >= '0' && character <= '9') {
            return static_cast<unsigned int>(character - '0');
        }
        if (character >= 'a' && character <= 'f') {
            return static_cast<unsigned int>(character - 'a' + 10);
        }
        if (character >= 'A' && character <= 'F') {
            return static_cast<unsigned int>(character - 'A' + 10);
        }
        throw std::runtime_error("Theme color contains a non-hexadecimal character.");
    };
    const auto channel = [&nibble, encodedColor](const std::size_t offset) {
        return static_cast<float>((nibble(encodedColor[offset]) << 4U) | nibble(encodedColor[offset + 1])) / 255.0F;
    };

    return {
        channel(1),
        channel(3),
        channel(5),
        encodedColor.size() == 9 ? channel(7) : 1.0F,
    };
}

[[nodiscard]] std::filesystem::path ValidateRelativeResourcePath(const std::string_view resourcePath) {
    const std::filesystem::path path(resourcePath);
    if (path.empty() || path.is_absolute()) {
        throw std::runtime_error("Theme resources must use a non-empty relative path.");
    }
    for (const auto& part : path) {
        if (part == "..") {
            throw std::runtime_error("Theme resources cannot leave the theme directory.");
        }
    }
    return path;
}

} // namespace

ThemeManifest ThemeManifest::LoadFromFile(const std::filesystem::path& manifestPath) {
    std::ifstream input(manifestPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open theme manifest.");
    }

    const std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    JsonReader reader{std::string_view(source)};
    const auto root = reader.ReadRootObject();
    reader.RequireEnd();

    const auto schemaVersion = RequireValue(root, "schemaVersion");
    if (schemaVersion != "1") {
        throw std::runtime_error("Unsupported theme manifest schema version.");
    }

    ThemeManifest manifest;
    manifest.id_ = RequireValue(root, "id");
    manifest.rootDirectory_ = manifestPath.parent_path();
    manifest.palette_ = {
        .panel = ParseColor(RequireValue(root, "palette.panel")),
        .accent = ParseColor(RequireValue(root, "palette.accent")),
        .heading = ParseColor(RequireValue(root, "palette.heading")),
        .detail = ParseColor(RequireValue(root, "palette.detail")),
        .instruction = ParseColor(RequireValue(root, "palette.instruction")),
    };

    for (const auto& [key, value] : root) {
        constexpr std::string_view prefix = "resources.";
        if (!key.starts_with(prefix)) {
            continue;
        }
        const auto resourceKey = key.substr(prefix.size());
        if (resourceKey.empty()) {
            throw std::runtime_error("Theme resource keys cannot be empty.");
        }
        manifest.resources_.emplace(resourceKey, ValidateRelativeResourcePath(value));
    }

    return manifest;
}

const std::string& ThemeManifest::Id() const noexcept {
    return id_;
}

const ThemePalette& ThemeManifest::Palette() const noexcept {
    return palette_;
}

std::filesystem::path ThemeManifest::ResolveResource(const std::string_view key) const {
    const auto found = resources_.find(std::string(key));
    if (found == resources_.end()) {
        throw std::runtime_error("Theme resource key was not found.");
    }

    const auto resolved = rootDirectory_ / found->second;
    if (!std::filesystem::exists(resolved)) {
        throw std::runtime_error("Theme resource file was not found.");
    }
    return resolved;
}

} // namespace pumpdx::assets
