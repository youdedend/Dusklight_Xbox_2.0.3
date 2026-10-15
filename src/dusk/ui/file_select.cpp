#include "dusk/ui/file_select.hpp"

#if defined(_UWP)
#include "dusk/ui/document.hpp"
#include "dusk/ui/file_browser.hpp"
#include "dusk/ui/ui.hpp"


#include <borealis/io.hpp>
#include <fmt/format.h>

#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#endif

namespace dusk::ui::file_select {

#if defined(_UWP)
namespace {

/**
 * Turns borealis filters (SDL-style "iso;gcm" or "*.iso;*.gcm" patterns) into lowercase extensions.
 * Returns an empty list, which accepts everything, if any filter asks for all files.
 */
std::vector<std::string> extensions_from(const std::vector<borealis::file_select::Filter>& filters) {
    std::vector<std::string> out;
    for (const auto& filter : filters) {
        std::string_view pattern = filter.pattern;
        while (!pattern.empty()) {
            const auto cut = pattern.find(';');
            std::string_view token = pattern.substr(0, cut);
            pattern = cut == std::string_view::npos ? std::string_view{} : pattern.substr(cut + 1);

            while (!token.empty() && (token.front() == ' ' || token.front() == '\t')) {
                token.remove_prefix(1);
            }
            while (!token.empty() && (token.back() == ' ' || token.back() == '\t')) {
                token.remove_suffix(1);
            }
            if (token == "*") {
                return {};
            }
            if (token.substr(0, 2) == "*.") {
                token.remove_prefix(2);
            } else if (!token.empty() && token.front() == '.') {
                token.remove_prefix(1);
            }
            if (token.empty()) {
                continue;
            }
            std::string ext(token);
            for (char& c : ext) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            out.push_back(std::move(ext));
        }
    }
    return out;
}

void deliver(const borealis::file_select::Callback& callback, std::optional<std::string> picked) {
    borealis::file_select::Result result;
    if (picked) {
        result.status = borealis::file_select::Status::Selected;
        result.locations.push_back(std::move(*picked));
    } else {
        result.status = borealis::file_select::Status::Canceled;
    }
    callback(std::move(result));
}

// Copies the export source into `folder` under `suggested`, adding " (2)", " (3)", ... if the name is
// taken. Mirrors the native save dialog, which would have asked before overwriting.
borealis::file_select::Result copy_export(
    const std::string& source, const std::string& folder, std::string suggested) {
    namespace fs = std::filesystem;
    borealis::file_select::Result result;
    std::error_code ec;

    const fs::path src = borealis::io::fs_path_from_utf8(source);
    if (suggested.empty()) {
        suggested = borealis::io::fs_path_to_string(src.filename());
    }
    if (suggested == "." || suggested == ".." || suggested.find_first_of("/\\") != std::string::npos) {
        result.status = borealis::file_select::Status::Failed;
        result.message = "Suggested name must be a safe file name";
        return result;
    }

    const fs::path dir = borealis::io::fs_path_from_utf8(folder);
    const fs::path name = borealis::io::fs_path_from_utf8(suggested);
    const std::string stem = borealis::io::fs_path_to_string(name.stem());
    const std::string ext = borealis::io::fs_path_to_string(name.extension());
    fs::path dest = dir / name;
    for (int n = 2; fs::exists(dest, ec); ++n) {
        dest = dir / borealis::io::fs_path_from_utf8(fmt::format("{} ({}){}", stem, n, ext));
    }

    fs::copy_file(src, dest, fs::copy_options::none, ec);
    if (ec) {
        result.status = borealis::file_select::Status::Failed;
        result.message = fmt::format("Could not write {}: {}", borealis::io::fs_path_to_string(dest),
            ec.message());
        return result;
    }

    result.status = borealis::file_select::Status::Selected;
    result.locations.push_back(borealis::io::fs_path_to_string(dest));
    return result;
}

void fail_no_host(const borealis::file_select::Callback& callback) {
    borealis::file_select::Result result;
    result.status = borealis::file_select::Status::Failed;
    result.message = "No screen is available to show the file browser";
    callback(std::move(result));
}

}  // namespace
#endif

void open_file(borealis::file_select::FileOptions options, borealis::file_select::Callback callback) {
#if defined(_UWP)
    if (!callback) {
        return;
    }
    Document* host = top_document();
    if (host == nullptr) {
        fail_no_host(callback);
        return;
    }
    host->push(std::make_unique<FileBrowser>(FileBrowserPurpose::File, "Select file",
        std::move(options.defaultLocation), extensions_from(options.filters),
        [callback](std::optional<std::string> picked) { deliver(callback, std::move(picked)); }));
#else
    borealis::file_select::open_file(std::move(options), std::move(callback));
#endif
}

void export_file(borealis::file_select::ExportOptions options, borealis::file_select::Callback callback) {
#if defined(_UWP)
    if (!callback) {
        return;
    }
    Document* host = top_document();
    if (host == nullptr) {
        fail_no_host(callback);
        return;
    }
    host->push(std::make_unique<FileBrowser>(FileBrowserPurpose::Folder, "Save to folder",
        std::string{}, std::vector<std::string>{},
        [callback, source = std::move(options.sourceLocation),
            suggested = std::move(options.suggestedName)](std::optional<std::string> folder) {
            if (!folder) {
                deliver(callback, std::nullopt);
                return;
            }
            callback(copy_export(source, *folder, suggested));
        }));
#else
    borealis::file_select::export_file(std::move(options), std::move(callback));
#endif
}

void open_folder(borealis::file_select::FolderOptions options, borealis::file_select::Callback callback) {
#if defined(_UWP)
    if (!callback) {
        return;
    }
    Document* host = top_document();
    if (host == nullptr) {
        fail_no_host(callback);
        return;
    }
    host->push(std::make_unique<FileBrowser>(FileBrowserPurpose::Folder, "Select folder",
        std::move(options.defaultLocation), std::vector<std::string>{},
        [callback](std::optional<std::string> picked) { deliver(callback, std::move(picked)); }));
#else
    borealis::file_select::open_folder(std::move(options), std::move(callback));
#endif
}

}  // namespace dusk::ui::file_select
