#pragma once

#include <borealis/file_select.hpp>

namespace dusk::ui::file_select {

/**
 * Same contract as borealis::file_select::open_file and open_folder.
 *
 * Desktop and mobile builds forward to borealis' native dialogs. The UWP build has no native
 * dialogs, so it opens the in-app FileBrowser and reports the choice through the same callback.
 * Call sites therefore do not need to know which platform they are running on.
 */
void open_file(borealis::file_select::FileOptions options, borealis::file_select::Callback callback);
void open_folder(borealis::file_select::FolderOptions options, borealis::file_select::Callback callback);

/**
 * Exports `sourceLocation` under `suggestedName` into a folder the user picks. On UWP the folder is
 * chosen in the in-app browser and the file is copied there, so the result reports the new path.
 */
void export_file(borealis::file_select::ExportOptions options, borealis::file_select::Callback callback);

}  // namespace dusk::ui::file_select
