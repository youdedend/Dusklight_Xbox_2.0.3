#pragma once

#include "button.hpp"
#include "pane.hpp"
#include "window.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace dusk::ui {

/// Whether the browser picks a file (filtered by extension) or a folder.
enum class FileBrowserPurpose : std::uint8_t { File, Folder };

/// Receives the chosen path, or nullopt if the browser was dismissed. Runs once, after the browser closes.
using FileBrowserDone = std::function<void(std::optional<std::string>)>;

/**
 * In-app file/folder picker. Used by the UWP build in place of native dialogs, which UWP does not
 * have. Reach it through dusk::ui::file_select rather than constructing it directly.
 */
class FileBrowser final : public WindowSmall {
public:
    /**
     * \param titleText        Heading shown at the top of the browser.
     * \param defaultLocation  Folder to start in. Falls back to a sensible default if missing.
     * \param extensions       Lowercase extensions without the dot (e.g. "iso"). Empty accepts every
     *                         file. Ignored for FileBrowserPurpose::Folder.
     * \param onDone           Called exactly once with the picked path, or nullopt on dismissal.
     */
    FileBrowser(FileBrowserPurpose purpose, std::string titleText, std::string defaultLocation,
        std::vector<std::string> extensions, FileBrowserDone onDone);
    ~FileBrowser() override;

    void update() override;
    bool focus() override;

protected:
    bool handle_nav_command(Rml::Event& event, NavCommand cmd) override;

private:
    enum class BrowseMode : std::uint8_t { SelectDrive, InDirectory };

    bool refresh_list();
    void on_up_pressed();
    void go_up();
    void select_drive_root(const std::filesystem::path& root);
    void enter_dir(const std::filesystem::path& p);
    void pick_file(const std::filesystem::path& p);
    void on_use_this_folder_pressed();
    void on_local_state_pressed();
    void on_new_folder_pressed();
    void finish(std::optional<std::string> picked);
    bool can_go_up() const;
    bool consume_vertical_nav_keys(Rml::Event& event, NavCommand cmd);

    FileBrowserPurpose mPurpose;
    std::vector<std::string> mExtensions;
    FileBrowserDone mOnDone;
    BrowseMode mBrowseMode = BrowseMode::InDirectory;
    std::filesystem::path mStartDirectory;
    std::filesystem::path mCurrent;
    Rml::Element* mPathDisplay = nullptr;
    Rml::Element* mErrorDisplay = nullptr;
    std::unique_ptr<Pane> mFilePane;
    std::unique_ptr<Button> mUpButton;
#if defined(_UWP)
    std::unique_ptr<Button> mLocalStateButton;
#endif
    std::unique_ptr<Button> mNewFolderButton;
    std::unique_ptr<Button> mUseFolderButton;
    std::unique_ptr<Button> mCancelButton;
};

}  // namespace dusk::ui
