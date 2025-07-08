#include "TextEditor.h"
#include <cstring> // strcmp için
#include <iostream>
#include <sstream>

ModernTextEditor::ModernTextEditor() : mode(INSERT_MODE), active_pane(0), split_direction(VERTICAL_SPLIT), shift_pressed(false), ctrl_pressed(false),
                                       showFileExplorer(true), showTerminal(true), fileExplorerWidth(200), terminalHeight(150),
                                       search_mode(false), replace_mode(false), current_search_result(-1), max_undo_levels(10),
                                       current_font_size(16)
{
    // İlk pane'i oluştur
    panes.push_back(EditorPane());
    panes[0].is_active = true;

    // Vector'lar için memory reserve et - Çok konservatif limitler
    panes.reserve(5);                // Maksimum 5 pane (daha az)
    undo_stack.reserve(10);          // Undo stack için (daha az)
    redo_stack.reserve(10);          // Redo stack için (daha az)
    terminal.output.reserve(50);     // Terminal output için (daha az)
    fileExplorer.items.reserve(100); // File explorer için (daha az)

    status_message = "INSERT MODE - Ctrl+C: Copy, Ctrl+V: Paste, Ctrl+A: Select All";

    // Font oluştur - 0xNerd Proto kalın
    hFont = CreateFont(current_font_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("0xNerd Proto"));

    // Siyah arka plan ve renkler
    bg_brush = CreateSolidBrush(RGB(0, 0, 0));           // Tam siyah arka plan
    status_brush = CreateSolidBrush(RGB(0, 122, 204));   // Mavi status bar
    cursor_brush = CreateSolidBrush(RGB(255, 165, 0));   // Turuncu cursor
    selection_brush = CreateSolidBrush(RGB(40, 62, 86)); // Seçim rengi
    explorer_brush = CreateSolidBrush(RGB(30, 30, 30));  // Koyu gri file explorer
    terminal_brush = CreateSolidBrush(RGB(20, 20, 20));  // Daha koyu terminal

    // Karakter boyutlarını hesapla
    calculateCharSize();

    // File explorer'ı başlat
    initializeFileExplorer();

    // Terminal'i başlat
    terminal.output.push_back("Glitch IDE Terminal");
    terminal.output.push_back("Type 'help' for commands");
    terminal.output.push_back("");
}

ModernTextEditor::~ModernTextEditor()
{
    DeleteObject(hFont);
    DeleteObject(bg_brush);
    DeleteObject(status_brush);
    DeleteObject(cursor_brush);
    DeleteObject(selection_brush);
    DeleteObject(explorer_brush);
    DeleteObject(terminal_brush);
}

void ModernTextEditor::setHwnd(HWND h)
{
    hwnd = h;
}

void ModernTextEditor::calculateCharSize()
{
    HDC hdc = GetDC(NULL);
    SelectObject(hdc, hFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    char_width = tm.tmAveCharWidth;
    char_height = tm.tmHeight;

    ReleaseDC(NULL, hdc);
}

void ModernTextEditor::updatePaneLayout()
{
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);

    int status_height = char_height + 10;
    int current_left = 0;
    int current_top = 0;
    int available_width = client_rect.right;
    int available_height = client_rect.bottom - status_height;

    // File Explorer layout (sol taraf)
    if (showFileExplorer)
    {
        fileExplorer.rect.left = 0;
        fileExplorer.rect.right = fileExplorerWidth;
        fileExplorer.rect.top = 0;
        fileExplorer.rect.bottom = available_height;

        current_left = fileExplorerWidth;
        available_width -= fileExplorerWidth;
    }

    // Terminal layout (alt kısım)
    if (showTerminal)
    {
        terminal.rect.left = current_left;
        terminal.rect.right = client_rect.right;
        terminal.rect.top = available_height - terminalHeight;
        terminal.rect.bottom = available_height;

        available_height -= terminalHeight;
    }

    // Editor panes layout (ortada kalan alan)
    int pane_count = panes.size();
    if (pane_count > 0)
    {
        if (split_direction == VERTICAL_SPLIT)
        {
            // Vertical split: paneller yan yana
            int pane_width = available_width / pane_count;
            for (int i = 0; i < pane_count; i++)
            {
                panes[i].rect.left = current_left + (i * pane_width);
                panes[i].rect.right = current_left + ((i + 1) * pane_width);
                panes[i].rect.top = current_top;
                panes[i].rect.bottom = current_top + available_height;
            }
        }
        else
        {
            // Horizontal split: paneller alt alta
            int pane_height = available_height / pane_count;
            for (int i = 0; i < pane_count; i++)
            {
                panes[i].rect.left = current_left;
                panes[i].rect.right = current_left + available_width;
                panes[i].rect.top = current_top + (i * pane_height);
                panes[i].rect.bottom = current_top + ((i + 1) * pane_height);
            }
        }
    }
}

void ModernTextEditor::handleKeyPress(WPARAM wParam)
{
    // Özel tuş durumlarını kontrol et
    shift_pressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    ctrl_pressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    // Ctrl kombinasyonları
    if (ctrl_pressed)
    {
        switch (wParam)
        {
        case 'A':
            selectAll();
            break;
        case 'C':
            copySelection();
            break;
        case 'V':
            pasteFromClipboard();
            break;
        case 'X':
            cutSelection();
            break;
        case 'O':
            openFile();
            break;
        case 'S':
            saveFile();
            break;
        case 'N':
            newFile();
            break;
        case 'Z':
            performUndo();
            break;
        case 'Y':
            performRedo();
            break;
        case 'F':
            startSearch();
            break;
        case 'H':
            startReplace();
            break;
        case 'J':
            // Toggle terminal (like VS Code)
            showTerminal = !showTerminal;
            updatePaneLayout();
            status_message = showTerminal ? "Terminal shown" : "Terminal hidden";
            break;
        case 'B':
            // Toggle file explorer (like VS Code)
            showFileExplorer = !showFileExplorer;
            updatePaneLayout();
            status_message = showFileExplorer ? "File Explorer shown" : "File Explorer hidden";
            break;
        case 'D':
            // Duplicate current line (Ctrl+Shift+D like VS Code)
            if (shift_pressed)
            {
                duplicateCurrentLine();
            }
            break;
        case 'K':
            // Delete current line (Ctrl+Shift+K like VS Code)
            if (shift_pressed)
            {
                deleteCurrentLine();
            }
            break;
        case 'G':
            // Go to line (like VS Code Ctrl+G)
            startGoToLine();
            break;
        case 'L':
            // Select current line (like VS Code Ctrl+L)
            selectCurrentLine();
            break;
        case VK_OEM_PLUS: // Plus key (+)
            // Zoom in
            increaseFontSize();
            break;
        case VK_OEM_MINUS: // Minus key (-)
            // Zoom out
            decreaseFontSize();
            break;
        case '0':
            // Reset zoom
            resetFontSize();
            break;
        case 'W':
            // Close current pane/file
            closePane();
            break;
        case 'P':
            // Quick file open (like VS Code Ctrl+P)
            startQuickOpen();
            break;
        case VK_OEM_3: // Backtick/Tilde key (`)
            // Quick terminal activation
            terminal.isActive = !terminal.isActive;
            status_message = terminal.isActive ? "Terminal activated" : "Terminal deactivated";
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            switchToPane(wParam - '1');
            break;
        case VK_PRIOR: // Page Up
            if (shift_pressed)
                updateSelection();
            else
                panes[active_pane].selection.clear();

            {
                int visible_lines = (panes[active_pane].rect.bottom - panes[active_pane].rect.top - 60) / (char_height + 2);
                panes[active_pane].cursor_row -= visible_lines;
                if (panes[active_pane].cursor_row < 0)
                    panes[active_pane].cursor_row = 0;
                panes[active_pane].cursor_col = std::min(panes[active_pane].cursor_col, (int)panes[active_pane].lines[panes[active_pane].cursor_row].length());
                ensureCursorVisible();
            }
            break;
        case VK_NEXT: // Page Down
            if (shift_pressed)
                updateSelection();
            else
                panes[active_pane].selection.clear();

            {
                int visible_lines = (panes[active_pane].rect.bottom - panes[active_pane].rect.top - 60) / (char_height + 2);
                panes[active_pane].cursor_row += visible_lines;
                if (panes[active_pane].cursor_row >= static_cast<int>(panes[active_pane].lines.size()))
                    panes[active_pane].cursor_row = static_cast<int>(panes[active_pane].lines.size()) - 1;
                panes[active_pane].cursor_col = std::min(panes[active_pane].cursor_col, (int)panes[active_pane].lines[panes[active_pane].cursor_row].length());
                ensureCursorVisible();
            }
            break;
        default:
            if (mode == INSERT_MODE)
                handleInsertMode(wParam);
            else
                handleCommandMode(wParam);
        }
    }
    else
    {
        if (mode == INSERT_MODE)
            handleInsertMode(wParam);
        else
            handleCommandMode(wParam);
    }

    InvalidateRect(hwnd, NULL, FALSE);
}

// Diğer fonksiyonlar buraya gelecek...
void ModernTextEditor::handleInsertMode(WPARAM wParam)
{
    EditorPane &pane = panes[active_pane];

    switch (wParam)
    {
    case VK_LEFT:
        if (shift_pressed)
            updateSelection();
        else
            pane.selection.clear();

        if (pane.cursor_col > 0)
            pane.cursor_col--;
        else if (pane.cursor_row > 0)
        {
            pane.cursor_row--;
            pane.cursor_col = pane.lines[pane.cursor_row].length();
        }
        ensureCursorVisible();
        break;

    case VK_RIGHT:
        if (shift_pressed)
            updateSelection();
        else
            pane.selection.clear();

        if (pane.cursor_col < static_cast<int>(pane.lines[pane.cursor_row].length()))
        {
            pane.cursor_col++;
        }
        else if (pane.cursor_row < static_cast<int>(pane.lines.size()) - 1)
        {
            pane.cursor_row++;
            pane.cursor_col = 0;
        }
        ensureCursorVisible();
        break;

    case VK_UP:
        if (shift_pressed)
            updateSelection();
        else
            pane.selection.clear();

        if (pane.cursor_row > 0)
        {
            pane.cursor_row--;
            pane.cursor_col = std::min(pane.cursor_col, (int)pane.lines[pane.cursor_row].length());
        }
        ensureCursorVisible();
        break;

    case VK_DOWN:
        if (shift_pressed)
            updateSelection();
        else
            pane.selection.clear();

        if (pane.cursor_row < static_cast<int>(pane.lines.size()) - 1)
        {
            pane.cursor_row++;
            pane.cursor_col = std::min(pane.cursor_col, (int)pane.lines[pane.cursor_row].length());
        }
        ensureCursorVisible();
        break;

    case VK_HOME:
        if (shift_pressed)
            updateSelection();
        else
            pane.selection.clear();
        pane.cursor_col = 0;
        ensureCursorVisible();
        break;

    case VK_END:
        if (shift_pressed)
            updateSelection();
        else
            pane.selection.clear();
        pane.cursor_col = pane.lines[pane.cursor_row].length();
        ensureCursorVisible();
        break;

    case VK_BACK:
        if (pane.selection.hasSelection())
        {
            deleteSelection();
        }
        else if (pane.cursor_col > 0)
        {
            saveUndoState("backspace");
            pane.lines[pane.cursor_row].erase(pane.cursor_col - 1, 1);
            pane.cursor_col--;
            pane.modified = true;
        }
        else if (pane.cursor_row > 0)
        {
            saveUndoState("backspace line");
            pane.cursor_col = pane.lines[pane.cursor_row - 1].length();
            pane.lines[pane.cursor_row - 1] += pane.lines[pane.cursor_row];
            pane.lines.erase(pane.lines.begin() + pane.cursor_row);
            pane.cursor_row--;
            pane.modified = true;
        }
        break;

    case VK_DELETE:
        if (pane.selection.hasSelection())
        {
            deleteSelection();
        }
        else if (pane.cursor_col < static_cast<int>(pane.lines[pane.cursor_row].length()))
        {
            saveUndoState("delete");
            pane.lines[pane.cursor_row].erase(pane.cursor_col, 1);
            pane.modified = true;
        }
        else if (pane.cursor_row < static_cast<int>(pane.lines.size()) - 1)
        {
            saveUndoState("delete line");
            pane.lines[pane.cursor_row] += pane.lines[pane.cursor_row + 1];
            pane.lines.erase(pane.lines.begin() + pane.cursor_row + 1);
            pane.modified = true;
        }
        break;

    case VK_RETURN:
        if (pane.selection.hasSelection())
            deleteSelection();

        {
            saveUndoState("new line");
            std::string current_line = pane.lines[pane.cursor_row];
            std::string left_part = current_line.substr(0, pane.cursor_col);
            std::string right_part = current_line.substr(pane.cursor_col);

            pane.lines[pane.cursor_row] = left_part;
            pane.lines.insert(pane.lines.begin() + pane.cursor_row + 1, right_part);

            pane.cursor_row++;
            pane.cursor_col = 0;
            pane.modified = true;
        }
        break;

    case VK_TAB:
        if (panes.size() > 1)
            switchActivePane();
        else
        {
            insertText("    "); // 4 boşluk
        }
        break;

    case VK_ESCAPE:
        // ESC tuşu artık mode değiştirmez, sadece selection'ı temizler
        pane.selection.clear();
        status_message = "Selection cleared";
        break;
    }
}

void ModernTextEditor::handleCommandMode(WPARAM wParam)
{
    if (wParam == VK_ESCAPE)
    {
        // Sadece command buffer'ı temizle, mode değişmez
        command_buffer.clear();
        status_message = "COMMAND MODE - Press ':' for commands";
        return;
    }

    if (wParam == VK_RETURN)
    {
        executeCommand();
        return;
    }

    if (wParam == VK_BACK && !command_buffer.empty())
    {
        command_buffer.pop_back();
        status_message = ":" + command_buffer;
        return;
    }

    // 'i' tuşuna basıldığında INSERT_MODE'a geç
    if (wParam == 'I' && command_buffer.empty())
    {
        mode = INSERT_MODE;
        status_message = "INSERT MODE - Ctrl+C: Copy, Ctrl+V: Paste, Ctrl+A: Select All";
        return;
    }

    // Karakter girişi
    if (wParam >= 'A' && wParam <= 'Z')
    {
        bool shift_pressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        char ch = shift_pressed ? (char)wParam : (char)(wParam + 32);
        command_buffer += ch;
        status_message = ":" + command_buffer;
    }
    else if (wParam >= '0' && wParam <= '9')
    {
        command_buffer += (char)wParam;
        status_message = ":" + command_buffer;
    }
    else if (wParam == VK_SPACE)
    {
        command_buffer += ' ';
        status_message = ":" + command_buffer;
    }
}

void ModernTextEditor::handleChar(WPARAM wParam)
{
    // Terminal aktifse terminal input'u handle et
    if (terminal.isActive)
    {
        handleTerminalInput((char)wParam);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    if (search_mode && wParam != VK_RETURN && wParam != VK_ESCAPE)
    {
        search_text += (char)wParam;
        status_message = "Search: " + search_text;
    }
    else if (replace_mode && wParam != VK_RETURN && wParam != VK_ESCAPE)
    {
        if (search_text.empty())
        {
            search_text += (char)wParam;
            status_message = "Replace: " + search_text;
        }
        else
        {
            replace_text += (char)wParam;
            status_message = "Replace: " + search_text + " -> " + replace_text;
        }
    }
    else if (mode == INSERT_MODE && wParam == ':')
    {
        // ':' tuşuna basıldığında COMMAND_MODE'a geç
        mode = COMMAND_MODE;
        command_buffer.clear();
        status_message = "COMMAND MODE - Type command and press Enter";
    }
    else if (mode == INSERT_MODE && wParam >= 32 && wParam <= 126)
    {
        if (panes[active_pane].selection.hasSelection())
            deleteSelection();

        char ch = (char)wParam;
        insertText(std::string(1, ch));
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

void ModernTextEditor::insertText(const std::string &text)
{
    // Memory protection - çok büyük text insertion'ları engelley
    if (text.length() > 500) // Daha küçük limit
    {
        status_message = "Text too large (max 500 chars)";
        return;
    }

    saveUndoState("insert text");
    EditorPane &pane = panes[active_pane];

    // Line length protection
    if (pane.lines[pane.cursor_row].length() + text.length() > 1000) // Daha kısa limit
    {
        status_message = "Line too long (max 1000 chars)";
        return;
    }

    pane.lines[pane.cursor_row].insert(pane.cursor_col, text);
    pane.cursor_col += text.length();
    pane.modified = true;
    ensureCursorVisible();
}

void ModernTextEditor::updateSelection()
{
    EditorPane &pane = panes[active_pane];

    if (!pane.selection.active)
    {
        pane.selection.start_row = pane.cursor_row;
        pane.selection.start_col = pane.cursor_col;
        pane.selection.active = true;
    }

    pane.selection.end_row = pane.cursor_row;
    pane.selection.end_col = pane.cursor_col;
}

void ModernTextEditor::selectAll()
{
    EditorPane &pane = panes[active_pane];
    pane.selection.active = true;
    pane.selection.start_row = 0;
    pane.selection.start_col = 0;
    pane.selection.end_row = pane.lines.size() - 1;
    pane.selection.end_col = pane.lines.back().length();

    status_message = "All text selected";
}

std::string ModernTextEditor::getSelectedText()
{
    EditorPane &pane = panes[active_pane];
    if (!pane.selection.hasSelection())
        return "";

    int start_row = std::min(pane.selection.start_row, pane.selection.end_row);
    int end_row = std::max(pane.selection.start_row, pane.selection.end_row);
    int start_col = (pane.selection.start_row <= pane.selection.end_row) ? pane.selection.start_col : pane.selection.end_col;
    int end_col = (pane.selection.start_row <= pane.selection.end_row) ? pane.selection.end_col : pane.selection.start_col;

    if (start_row == end_row)
    {
        return pane.lines[start_row].substr(start_col, end_col - start_col);
    }

    std::string result;
    for (int i = start_row; i <= end_row; i++)
    {
        if (i == start_row)
            result += pane.lines[i].substr(start_col);
        else if (i == end_row)
            result += pane.lines[i].substr(0, end_col);
        else
            result += pane.lines[i];

        if (i < end_row)
            result += "\r\n";
    }

    return result;
}

void ModernTextEditor::copySelection()
{
    std::string selected = getSelectedText();
    if (!selected.empty())
    {
        copyToClipboard(selected);
        status_message = "Copied to clipboard";
    }
}

void ModernTextEditor::cutSelection()
{
    std::string selected = getSelectedText();
    if (!selected.empty())
    {
        copyToClipboard(selected);
        deleteSelection();
        status_message = "Cut to clipboard";
    }
}

void ModernTextEditor::deleteSelection()
{
    EditorPane &pane = panes[active_pane];
    if (!pane.selection.hasSelection())
        return;

    saveUndoState("delete selection");

    int start_row = std::min(pane.selection.start_row, pane.selection.end_row);
    int end_row = std::max(pane.selection.start_row, pane.selection.end_row);
    int start_col = (pane.selection.start_row <= pane.selection.end_row) ? pane.selection.start_col : pane.selection.end_col;
    int end_col = (pane.selection.start_row <= pane.selection.end_row) ? pane.selection.end_col : pane.selection.start_col;

    if (start_row == end_row)
    {
        pane.lines[start_row].erase(start_col, end_col - start_col);
    }
    else
    {
        pane.lines[start_row] = pane.lines[start_row].substr(0, start_col) +
                                pane.lines[end_row].substr(end_col);
        pane.lines.erase(pane.lines.begin() + start_row + 1,
                         pane.lines.begin() + end_row + 1);
    }

    pane.cursor_row = start_row;
    pane.cursor_col = start_col;
    pane.selection.clear();
    pane.modified = true;
    ensureCursorVisible();
}

void ModernTextEditor::copyToClipboard(const std::string &text)
{
    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hg)
        {
            memcpy(GlobalLock(hg), text.c_str(), text.size() + 1);
            GlobalUnlock(hg);
            SetClipboardData(CF_TEXT, hg);
        }
        CloseClipboard();
    }
}

void ModernTextEditor::pasteFromClipboard()
{
    if (OpenClipboard(hwnd))
    {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData)
        {
            char *text = (char *)GlobalLock(hData);
            if (text)
            {
                if (panes[active_pane].selection.hasSelection())
                    deleteSelection();

                std::string paste_text = text;
                insertMultilineText(paste_text);
                status_message = "Pasted from clipboard";
            }
            GlobalUnlock(hData);
        }
        CloseClipboard();
    }
}

void ModernTextEditor::insertMultilineText(const std::string &text)
{
    EditorPane &pane = panes[active_pane];
    std::istringstream iss(text);
    std::string line;
    bool first_line = true;

    while (std::getline(iss, line))
    {
        if (!first_line)
        {
            // Yeni satır ekle
            std::string current_line = pane.lines[pane.cursor_row];
            std::string left_part = current_line.substr(0, pane.cursor_col);
            std::string right_part = current_line.substr(pane.cursor_col);

            pane.lines[pane.cursor_row] = left_part;
            pane.lines.insert(pane.lines.begin() + pane.cursor_row + 1, right_part);
            pane.cursor_row++;
            pane.cursor_col = 0;
        }

        insertText(line);
        first_line = false;
    }
}

void ModernTextEditor::executeCommand()
{
    if (command_buffer == "q" || command_buffer == "quit")
    {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    else if (command_buffer == "w" || command_buffer == "write")
    {
        saveFile();
    }
    else if (command_buffer == "wq")
    {
        saveFile();
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    else if (command_buffer == "vsplit" || command_buffer == "vsp")
    {
        verticalSplit();
    }
    else if (command_buffer == "split" || command_buffer == "sp")
    {
        horizontalSplit();
    }
    else if (command_buffer == "close")
    {
        closePane();
    }
    else if (command_buffer.substr(0, 5) == "goto ")
    {
        // Go to line command
        std::string lineNumStr = command_buffer.substr(5);
        try
        {
            int lineNum = std::stoi(lineNumStr) - 1; // 1-based to 0-based
            EditorPane &pane = panes[active_pane];
            if (lineNum >= 0 && lineNum < static_cast<int>(pane.lines.size()))
            {
                pane.cursor_row = lineNum;
                pane.cursor_col = 0;
                ensureCursorVisible();
                status_message = "Jumped to line " + std::to_string(lineNum + 1);
            }
            else
            {
                status_message = "Line number out of range (1-" + std::to_string(pane.lines.size()) + ")";
            }
        }
        catch (...)
        {
            status_message = "Invalid line number";
        }
    }
    else
    {
        status_message = "Unknown command: " + command_buffer;
    }

    command_buffer.clear();
    // Mode değişmez, hangi moddaysak o modda kalırız
}

void ModernTextEditor::verticalSplit()
{
    EditorPane new_pane;
    new_pane.lines = panes[active_pane].lines;
    new_pane.filename = panes[active_pane].filename;
    panes.push_back(new_pane);
    split_direction = VERTICAL_SPLIT;
    updatePaneLayout();
    status_message = "Vertical split created";
}

void ModernTextEditor::horizontalSplit()
{
    EditorPane new_pane;
    new_pane.lines = panes[active_pane].lines;
    new_pane.filename = panes[active_pane].filename;
    panes.push_back(new_pane);
    split_direction = HORIZONTAL_SPLIT;
    updatePaneLayout();
    status_message = "Horizontal split created";
}

void ModernTextEditor::closePane()
{
    if (panes.size() > 1)
    {
        panes.erase(panes.begin() + active_pane);
        if (static_cast<size_t>(active_pane) >= panes.size())
            active_pane = static_cast<int>(panes.size()) - 1;
        updatePanes();
        updatePaneLayout();
    }
}

void ModernTextEditor::switchActivePane()
{
    if (panes.size() > 1)
    {
        panes[active_pane].is_active = false;
        active_pane = (active_pane + 1) % panes.size();
        panes[active_pane].is_active = true;
        status_message = "Switched to pane " + std::to_string(active_pane + 1);
    }
}

void ModernTextEditor::updatePanes()
{
    for (size_t i = 0; i < panes.size(); i++)
    {
        panes[i].is_active = (static_cast<int>(i) == active_pane);
    }
}

void ModernTextEditor::switchToPane(int pane_index)
{
    if (pane_index >= 0 && static_cast<size_t>(pane_index) < panes.size())
    {
        active_pane = pane_index;
        updatePanes();
        status_message = "Switched to pane " + std::to_string(pane_index + 1);
    }
}

void ModernTextEditor::newFile()
{
    EditorPane new_pane;
    panes[active_pane] = new_pane;
    status_message = "New file created";
}

void ModernTextEditor::openFile()
{
    OPENFILENAME ofn;
    char szFile[260] = {0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        loadFile(szFile);
    }
}

void ModernTextEditor::loadFile(const std::string &filename)
{
    FILE *file = fopen(filename.c_str(), "r");
    if (file)
    {
        EditorPane &pane = panes[active_pane];
        pane.lines.clear();

        char line[2048]; // Buffer boyutunu sınırla
        int lineCount = 0;

        while (fgets(line, sizeof(line), file) && lineCount < 5000) // Maksimum 5000 satır
        {
            std::string str_line = line;
            if (!str_line.empty() && str_line.back() == '\n')
                str_line.pop_back();
            if (!str_line.empty() && str_line.back() == '\r')
                str_line.pop_back();

            // Line length protection
            if (str_line.length() > 1000) // Daha kısa satır limiti
            {
                str_line = str_line.substr(0, 1000) + "... [line truncated]";
            }

            pane.lines.push_back(str_line);
            lineCount++;
        }

        if (pane.lines.empty())
            pane.lines.push_back("");

        pane.filename = filename;
        pane.cursor_row = 0;
        pane.cursor_col = 0;
        pane.modified = false;

        fclose(file);

        if (lineCount >= 5000)
        {
            status_message = "File loaded (truncated at 5000 lines): " + filename;
        }
        else
        {
            status_message = "File loaded: " + filename;
        }
    }
    else
    {
        status_message = "Error opening file: " + filename;
    }
}

void ModernTextEditor::saveFile()
{
    EditorPane &pane = panes[active_pane];

    if (pane.filename == "Untitled")
    {
        OPENFILENAME ofn;
        char szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "All Files\0*.*\0Text Files\0*.txt\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn))
        {
            pane.filename = szFile;
        }
        else
        {
            return;
        }
    }

    FILE *file = fopen(pane.filename.c_str(), "w");
    if (file)
    {
        for (const auto &line : pane.lines)
        {
            fprintf(file, "%s\n", line.c_str());
        }
        fclose(file);
        pane.modified = false;
        status_message = "File saved: " + pane.filename;
    }
    else
    {
        status_message = "Error saving file: " + pane.filename;
    }
}

void ModernTextEditor::paint(HDC hdc)
{
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);

    // TAMAMEN SİYAH YAP - ZORLA!
    HBRUSH pure_black = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &client_rect, pure_black);

    // Tüm HDC'yi siyah yap
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, RGB(0, 0, 0));

    // Font seç
    SelectObject(hdc, hFont);

    // File Explorer çiz (sol taraf)
    if (showFileExplorer)
    {
        drawFileExplorer(hdc);
    }

    // Editor panes çiz (orta alan)
    for (size_t i = 0; i < panes.size(); i++)
    {
        // Pane alanını da zorla siyah yap
        FillRect(hdc, &panes[i].rect, pure_black);
        drawPane(hdc, panes[i], static_cast<int>(i));
    }

    // Terminal çiz (alt kısım)
    if (showTerminal)
    {
        drawTerminal(hdc);
    }

    // Status bar çiz
    drawStatusBar(hdc);

    DeleteObject(pure_black);
}

void ModernTextEditor::drawPane(HDC hdc, const EditorPane &pane, int pane_index)
{
    // PANE'İ TAMAMEN SİYAH YAP
    HBRUSH total_black = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &pane.rect, total_black);

    // HDC ayarları - HER ŞEY SİYAH ARKA PLAN
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, RGB(0, 0, 0));

    // Metin içeriğini çiz
    int line_y = pane.rect.top + 30;
    int line_height = char_height + 2;

    for (size_t i = static_cast<size_t>(pane.scroll_top);
         i < pane.lines.size() && line_y < pane.rect.bottom - line_height;
         i++)
    {
        const std::string &line = pane.lines[i];

        // TAMAMEN SİYAH SATIR ARKA PLANI
        RECT full_line = {pane.rect.left, line_y, pane.rect.right, line_y + line_height};
        FillRect(hdc, &full_line, total_black);

        // Turuncu satır numarası
        SetTextColor(hdc, RGB(255, 165, 0));
        std::string line_num = std::to_string(i + 1);
        while (line_num.length() < 4)
            line_num = " " + line_num;
        TextOutA(hdc, pane.rect.left + 5, line_y, line_num.c_str(), line_num.length());

        // Linux terminal yeşili metin
        SetTextColor(hdc, RGB(0, 255, 0));
        int text_x = pane.rect.left + 50;
        TextOutA(hdc, text_x, line_y, line.c_str(), line.length());

        // Cursor çiz
        if (pane.is_active && static_cast<int>(i) == pane.cursor_row)
        {
            int cursor_x = text_x + (pane.cursor_col * char_width);
            RECT cursor_rect = {cursor_x, line_y, cursor_x + 2, line_y + line_height};
            FillRect(hdc, &cursor_rect, cursor_brush);
        }

        line_y += line_height;
    }

    DeleteObject(total_black);
}

void ModernTextEditor::drawSelection(HDC hdc, const EditorPane &pane, int current_line, int line_y)
{
    if (!pane.selection.hasSelection())
        return;

    int start_row = std::min(pane.selection.start_row, pane.selection.end_row);
    int end_row = std::max(pane.selection.start_row, pane.selection.end_row);
    int start_col = (start_row == pane.selection.start_row) ? pane.selection.start_col : pane.selection.end_col;
    int end_col = (end_row == pane.selection.end_row) ? pane.selection.end_col : pane.selection.start_col;

    if (current_line < start_row || current_line > end_row)
        return;

    int text_x = pane.rect.left + 50;
    int sel_start = text_x;
    int sel_end = text_x + pane.lines[current_line].length() * char_width;

    if (current_line == start_row)
        sel_start = text_x + (start_col * char_width);
    if (current_line == end_row)
        sel_end = text_x + (end_col * char_width);

    RECT sel_rect = {sel_start, line_y, sel_end, line_y + char_height + 2};
    FillRect(hdc, &sel_rect, selection_brush);
}

void ModernTextEditor::drawStatusBar(HDC hdc)
{
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);

    RECT status_rect = client_rect;
    status_rect.top = client_rect.bottom - (char_height + 10);

    FillRect(hdc, &status_rect, status_brush);

    SetBkMode(hdc, OPAQUE);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 122, 204)); // Status bar arka planı mavi

    EditorPane &current_pane = panes[active_pane];
    std::string left_status = "Ln " + std::to_string(current_pane.cursor_row + 1) +
                              ", Col " + std::to_string(current_pane.cursor_col + 1);

    if (mode == COMMAND_MODE && !command_buffer.empty())
    {
        left_status = ":" + command_buffer;
    }
    else if (mode == INSERT_MODE)
    {
        left_status += " | " + status_message;
    }

    TextOutA(hdc, 10, status_rect.top + 5, left_status.c_str(), left_status.length());

    std::string mode_info = (mode == INSERT_MODE) ? "-- INSERT --" : "-- COMMAND --";
    if (mode == COMMAND_MODE && !command_buffer.empty())
    {
        mode_info = "-- COMMAND: " + command_buffer + " --";
    }
    SIZE text_size;
    GetTextExtentPoint32A(hdc, mode_info.c_str(), mode_info.length(), &text_size);
    TextOutA(hdc, client_rect.right - text_size.cx - 10, status_rect.top + 5,
             mode_info.c_str(), mode_info.length());
}

void ModernTextEditor::handleResize()
{
    updatePaneLayout();
    InvalidateRect(hwnd, NULL, FALSE);
}

void ModernTextEditor::handleMouseClick(int x, int y)
{
    // File Explorer'a tıklama kontrolü
    if (showFileExplorer && x >= fileExplorer.rect.left && x <= fileExplorer.rect.right &&
        y >= fileExplorer.rect.top && y <= fileExplorer.rect.bottom)
    {
        handleFileExplorerClick(x, y);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    // Terminal'e tıklama kontrolü
    if (showTerminal && x >= terminal.rect.left && x <= terminal.rect.right &&
        y >= terminal.rect.top && y <= terminal.rect.bottom)
    {
        handleTerminalClick(x, y);
        InvalidateRect(hwnd, NULL, FALSE);
        return;
    }

    // Editor pane'lere tıklama kontrolü
    for (size_t i = 0; i < panes.size(); i++)
    {
        if (x >= panes[i].rect.left && x <= panes[i].rect.right &&
            y >= panes[i].rect.top && y <= panes[i].rect.bottom)
        {
            // Bu pane'i aktif yap
            active_pane = i;
            updatePanes();
            terminal.isActive = false; // Terminal'i deaktif et
            status_message = "Editor pane " + std::to_string(i + 1) + " activated";

            // Cursor pozisyonunu güncelle
            int text_x = panes[i].rect.left + 50;
            int line_y = panes[i].rect.top + 30;
            int line_height = char_height + 2;

            if (x >= text_x && y >= line_y)
            {
                int clicked_line = (y - line_y) / line_height + panes[i].scroll_top;
                int clicked_col = (x - text_x) / char_width;

                if (clicked_line >= 0 && static_cast<size_t>(clicked_line) < panes[i].lines.size())
                {
                    panes[i].cursor_row = clicked_line;
                    panes[i].cursor_col = std::min(clicked_col, (int)panes[i].lines[clicked_line].length());
                    ensureCursorVisible();
                }
            }

            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
    }
}

// Search & Replace functions
void ModernTextEditor::startSearch()
{
    search_mode = true;
    replace_mode = false;
    search_text = "";
    status_message = "Search: ";
    mode = COMMAND_MODE;
}

void ModernTextEditor::startReplace()
{
    replace_mode = true;
    search_mode = false;
    search_text = "";
    replace_text = "";
    status_message = "Replace: ";
    mode = COMMAND_MODE;
}

void ModernTextEditor::performSearch()
{
    EditorPane &pane = panes[active_pane];
    int start_row = pane.cursor_row;
    int start_col = pane.cursor_col;

    // Mevcut satırdan arama başlat
    for (size_t i = static_cast<size_t>(start_row); i < pane.lines.size(); i++)
    {
        size_t search_start = (static_cast<int>(i) == start_row) ? static_cast<size_t>(start_col) : 0;
        size_t pos = pane.lines[i].find(search_text, search_start);
        if (pos != std::string::npos)
        {
            pane.cursor_row = static_cast<int>(i);
            pane.cursor_col = static_cast<int>(pos);
            current_search_result = static_cast<int>(i);
            ensureCursorVisible();
            status_message = "Found: " + search_text;
            return;
        }
    }

    // Baştan arama yap
    for (size_t i = 0; i <= static_cast<size_t>(start_row); i++)
    {
        size_t pos = pane.lines[i].find(search_text, 0);
        if (pos != std::string::npos && (i < static_cast<size_t>(start_row) || pos < static_cast<size_t>(start_col)))
        {
            pane.cursor_row = static_cast<int>(i);
            pane.cursor_col = static_cast<int>(pos);
            current_search_result = static_cast<int>(i);
            ensureCursorVisible();
            status_message = "Found: " + search_text;
            return;
        }
    }

    status_message = "Not found: " + search_text;
}

void ModernTextEditor::performReplace()
{
    if (search_text.empty())
        return;

    EditorPane &pane = panes[active_pane];
    std::string &current_line = pane.lines[pane.cursor_row];

    size_t pos = current_line.find(search_text, static_cast<size_t>(pane.cursor_col));
    if (pos == static_cast<size_t>(pane.cursor_col))
    {
        current_line.replace(pos, search_text.length(), replace_text);
        pane.cursor_col = pos + replace_text.length();
        pane.modified = true;
        ensureCursorVisible();
        status_message = "Replaced: " + search_text + " -> " + replace_text;
    }
    else
    {
        performSearch(); // Sonraki bulguyu bul
    }
}

// Undo/Redo functions
void ModernTextEditor::saveUndoState(const std::string &operation)
{
    EditorPane &pane = panes[active_pane];
    UndoState state;
    state.lines = pane.lines;
    state.cursor_row = pane.cursor_row;
    state.cursor_col = pane.cursor_col;
    state.operation = operation;

    undo_stack.push_back(state);

    // Undo stack boyutunu sınırla (çok küçük)
    if (undo_stack.size() > static_cast<size_t>(10)) // Maksimum 10 undo
    {
        undo_stack.erase(undo_stack.begin(), undo_stack.begin() + 5);
    }

    // Redo stack'i temizle
    redo_stack.clear();
}

void ModernTextEditor::performUndo()
{
    if (undo_stack.empty())
    {
        status_message = "Nothing to undo";
        return;
    }

    EditorPane &pane = panes[active_pane];

    // Mevcut durumu redo stack'e kaydet
    UndoState current_state;
    current_state.lines = pane.lines;
    current_state.cursor_row = pane.cursor_row;
    current_state.cursor_col = pane.cursor_col;
    current_state.operation = "redo";
    redo_stack.push_back(current_state);

    // Son undo state'i geri yükle
    UndoState last_state = undo_stack.back();
    undo_stack.pop_back();

    pane.lines = last_state.lines;
    pane.cursor_row = last_state.cursor_row;
    pane.cursor_col = last_state.cursor_col;
    pane.modified = true;

    status_message = "Undone: " + last_state.operation;
}

void ModernTextEditor::performRedo()
{
    if (redo_stack.empty())
    {
        status_message = "Nothing to redo";
        return;
    }

    EditorPane &pane = panes[active_pane];

    // Mevcut durumu undo stack'e kaydet
    UndoState current_state;
    current_state.lines = pane.lines;
    current_state.cursor_row = pane.cursor_row;
    current_state.cursor_col = pane.cursor_col;
    current_state.operation = "undo";
    undo_stack.push_back(current_state);

    // Son redo state'i geri yükle
    UndoState last_state = redo_stack.back();
    redo_stack.pop_back();

    pane.lines = last_state.lines;
    pane.cursor_row = last_state.cursor_row;
    pane.cursor_col = last_state.cursor_col;
    pane.modified = true;

    status_message = "Redone: " + last_state.operation;
}

// File Explorer functions
void ModernTextEditor::initializeFileExplorer()
{
    // Mevcut dizini al
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    fileExplorer.currentPath = currentDir;
    refreshFileExplorer();
}

void ModernTextEditor::refreshFileExplorer()
{
    fileExplorer.items.clear();
    loadDirectory(fileExplorer.currentPath, fileExplorer.items, 0);
}

void ModernTextEditor::loadDirectory(const std::string &path, std::vector<FileItem> &items, int level)
{
    // Recursive depth protection - çok derin gitmesin
    if (level > 3)
        return;

    // File count protection - çok fazla dosya yüklemesin
    if (items.size() > 50) // Daha az dosya limiti
        return;

    WIN32_FIND_DATAA findData;
    std::string searchPath = path + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    int fileCount = 0;
    do
    {
        // File count limit
        if (fileCount++ > 30) // Daha az dosya per directory
            break;

        // . ve .. dizinlerini atla
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
            continue;

        FileItem item;
        item.name = findData.cFileName;
        item.fullPath = path + "\\" + findData.cFileName;
        item.level = level;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            item.type = FILE_TYPE_FOLDER;
            item.isExpanded = false;
        }
        else
        {
            item.type = getFileType(findData.cFileName);
        }

        items.push_back(item);
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

FileType ModernTextEditor::getFileType(const std::string &filename)
{
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos)
        return FILE_TYPE_OTHER;

    std::string ext = filename.substr(dot + 1);

    if (ext == "cpp" || ext == "cc" || ext == "cxx")
        return FILE_TYPE_CPP;
    else if (ext == "h" || ext == "hpp" || ext == "hxx")
        return FILE_TYPE_HEADER;
    else if (ext == "py")
        return FILE_TYPE_PYTHON;
    else if (ext == "js" || ext == "ts")
        return FILE_TYPE_JAVASCRIPT;
    else if (ext == "txt" || ext == "md" || ext == "log")
        return FILE_TYPE_TEXT;
    else
        return FILE_TYPE_OTHER;
}

void ModernTextEditor::drawFileExplorer(HDC hdc)
{
    // File explorer arka planını çiz
    FillRect(hdc, &fileExplorer.rect, explorer_brush);

    // Başlık çiz
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, RGB(30, 30, 30));
    SetTextColor(hdc, RGB(200, 200, 200));

    std::string title = "FILE EXPLORER";
    TextOutA(hdc, fileExplorer.rect.left + 5, fileExplorer.rect.top + 5, title.c_str(), title.length());

    // Mevcut dizin
    std::string currentPath = "Path: " + fileExplorer.currentPath;
    TextOutA(hdc, fileExplorer.rect.left + 5, fileExplorer.rect.top + 25, currentPath.c_str(), currentPath.length());

    // Dosya listesi
    int y = fileExplorer.rect.top + 50;
    int line_height = char_height + 2;

    for (size_t i = static_cast<size_t>(fileExplorer.scrollTop);
         i < fileExplorer.items.size() && y < fileExplorer.rect.bottom - line_height;
         i++)
    {
        const FileItem &item = fileExplorer.items[i];

        // Seçili item arka planı
        if (static_cast<int>(i) == fileExplorer.selectedIndex)
        {
            RECT selRect = {fileExplorer.rect.left, y, fileExplorer.rect.right, y + line_height};
            HBRUSH selBrush = CreateSolidBrush(RGB(50, 100, 150));
            FillRect(hdc, &selRect, selBrush);
            DeleteObject(selBrush);
        }

        // Indent for level
        int x_offset = fileExplorer.rect.left + 5 + (item.level * 15);

        // Icon ve text rengi
        COLORREF textColor = RGB(200, 200, 200);
        std::string prefix = "";

        switch (item.type)
        {
        case FILE_TYPE_FOLDER:
            prefix = item.isExpanded ? "[+] " : "[-] ";
            textColor = RGB(255, 255, 100);
            break;
        case FILE_TYPE_CPP:
            prefix = "[C] ";
            textColor = RGB(100, 150, 255);
            break;
        case FILE_TYPE_HEADER:
            prefix = "[H] ";
            textColor = RGB(150, 100, 255);
            break;
        case FILE_TYPE_PYTHON:
            prefix = "[P] ";
            textColor = RGB(100, 255, 100);
            break;
        case FILE_TYPE_JAVASCRIPT:
            prefix = "[J] ";
            textColor = RGB(255, 255, 100);
            break;
        default:
            prefix = "[F] ";
            break;
        }

        SetTextColor(hdc, textColor);
        std::string displayName = prefix + item.name;
        TextOutA(hdc, x_offset, y, displayName.c_str(), displayName.length());

        y += line_height;
    }
}

// Terminal functions
void ModernTextEditor::drawTerminal(HDC hdc)
{
    // Terminal arka planını çiz
    FillRect(hdc, &terminal.rect, terminal_brush);

    // Başlık çiz
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, RGB(20, 20, 20));
    SetTextColor(hdc, RGB(0, 255, 0)); // Terminal yeşili

    std::string title = "TERMINAL";
    TextOutA(hdc, terminal.rect.left + 5, terminal.rect.top + 5, title.c_str(), title.length());

    // Terminal output çiz
    int y = terminal.rect.top + 25;
    int line_height = char_height + 2;

    // Output lines
    for (size_t i = static_cast<size_t>(terminal.scrollTop);
         i < terminal.output.size() && y < terminal.rect.bottom - line_height * 2;
         i++)
    {
        SetTextColor(hdc, RGB(200, 200, 200));
        TextOutA(hdc, terminal.rect.left + 5, y, terminal.output[i].c_str(), terminal.output[i].length());
        y += line_height;
    }

    // Current input line
    if (terminal.isActive)
    {
        SetTextColor(hdc, RGB(0, 255, 0));
        std::string prompt = "> " + terminal.currentInput;
        TextOutA(hdc, terminal.rect.left + 5, terminal.rect.bottom - line_height - 5,
                 prompt.c_str(), prompt.length());

        // Cursor
        int cursor_x = terminal.rect.left + 5 + (prompt.length() * char_width);
        RECT cursor_rect = {cursor_x, terminal.rect.bottom - line_height - 5,
                            cursor_x + 2, terminal.rect.bottom - 5};
        FillRect(hdc, &cursor_rect, cursor_brush);
    }
}

void ModernTextEditor::handleTerminalInput(char ch)
{
    if (ch == '\r' || ch == '\n')
    {
        // Enter tuşu - komutu çalıştır
        std::string command = terminal.currentInput;
        terminal.output.push_back("> " + command);
        executeTerminalCommand(command);
        terminal.currentInput.clear();
    }
    else if (ch == '\b')
    {
        // Backspace
        if (!terminal.currentInput.empty())
            terminal.currentInput.pop_back();
    }
    else if (ch >= 32 && ch <= 126)
    {
        // Normal karakter
        terminal.currentInput += ch;
    }
}

void ModernTextEditor::executeTerminalCommand(const std::string &command)
{
    if (command.empty())
        return;

    // Özel terminal komutları
    if (command == "help")
    {
        addTerminalOutput("Glitch IDE Terminal Commands:");
        addTerminalOutput("  help     - Show this help");
        addTerminalOutput("  clear    - Clear terminal");
        addTerminalOutput("  exit     - Deactivate terminal");
        addTerminalOutput("");
        addTerminalOutput("All other commands are executed via PowerShell/CMD");
        return;
    }
    else if (command == "clear")
    {
        terminal.output.clear();
        addTerminalOutput("Terminal cleared");
        return;
    }
    else if (command == "exit")
    {
        terminal.isActive = false;
        addTerminalOutput("Terminal deactivated");
        return;
    }

    // Gerçek sistem komutlarını çalıştır
    std::string fullCommand;

    // Windows üzerinde PowerShell kullan
    fullCommand = "powershell.exe -Command \"" + command + "\" 2>&1";

    // Pipe ile komutu çalıştır
    FILE *pipe = _popen(fullCommand.c_str(), "r");
    if (pipe)
    {
        char buffer[512]; // Buffer boyutunu küçültüldü
        std::string result;
        int lineCount = 0;

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr && lineCount < 20) // Maksimum 20 satır
        {
            result += buffer;
            lineCount++;

            // Memory protection
            if (result.length() > 5000) // 5KB limit
                break;
        }

        int exitCode = _pclose(pipe);

        // Çıktıyı satır satır ekle
        std::istringstream stream(result);
        std::string line;
        bool hasOutput = false;
        int outputLines = 0;

        while (std::getline(stream, line) && outputLines < 15) // Maksimum 15 output line
        {
            // Carriage return karakterlerini temizle
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            addTerminalOutput(line);
            hasOutput = true;
            outputLines++;
        }

        if (!hasOutput)
        {
            if (exitCode == 0)
                addTerminalOutput("Command executed successfully");
            else
                addTerminalOutput("Command failed with exit code: " + std::to_string(exitCode));
        }

        if (lineCount >= 20)
        {
            addTerminalOutput("... output truncated (too long)");
        }
    }
}

void ModernTextEditor::addTerminalOutput(const std::string &text)
{
    terminal.output.push_back(text);

    // Çok fazla satır varsa eski olanları sil (en agresif)
    if (terminal.output.size() > 30) // 100'den 30'a düşürdük
    {
        terminal.output.erase(terminal.output.begin(), terminal.output.begin() + 15);
    }

    // Auto-scroll to bottom
    int visible_lines = (terminal.rect.bottom - terminal.rect.top - 30) / (char_height + 2);
    if (static_cast<int>(terminal.output.size()) > visible_lines)
    {
        terminal.scrollTop = static_cast<int>(terminal.output.size()) - visible_lines;
    }
}

void ModernTextEditor::handleTerminalClick(int x, int y)
{
    // Terminal alanına tıklandığında aktif yap
    if (x >= terminal.rect.left && x <= terminal.rect.right &&
        y >= terminal.rect.top && y <= terminal.rect.bottom)
    {
        terminal.isActive = true;
        status_message = "Terminal activated - Type commands";
    }
    else
    {
        terminal.isActive = false;
    }
}

void ModernTextEditor::handleFileExplorerClick(int x, int y)
{
    if (x < fileExplorer.rect.left || x > fileExplorer.rect.right ||
        y < fileExplorer.rect.top || y > fileExplorer.rect.bottom)
        return;

    // Hangi dosya/klasöre tıklandığını bul
    int clicked_y = y - fileExplorer.rect.top - 50;
    int line_height = char_height + 2;
    int clicked_index = fileExplorer.scrollTop + (clicked_y / line_height);

    if (clicked_index >= 0 && clicked_index < static_cast<int>(fileExplorer.items.size()))
    {
        fileExplorer.selectedIndex = clicked_index;
        FileItem &item = fileExplorer.items[clicked_index];

        if (item.type == FILE_TYPE_FOLDER)
        {
            // Klasöre çift tıklama - dizin değiştir
            fileExplorer.currentPath = item.fullPath;
            SetCurrentDirectoryA(fileExplorer.currentPath.c_str());
            refreshFileExplorer();
            status_message = "Changed directory to: " + item.name;
        }
        else
        {
            // Dosya - editörde aç
            loadFile(item.fullPath);
            status_message = "Opened file: " + item.name;
        }
    }
}

// New utility functions
void ModernTextEditor::duplicateCurrentLine()
{
    EditorPane &pane = panes[active_pane];
    if (pane.cursor_row < static_cast<int>(pane.lines.size()))
    {
        saveUndoState("duplicate line");
        std::string currentLine = pane.lines[pane.cursor_row];
        pane.lines.insert(pane.lines.begin() + pane.cursor_row + 1, currentLine);
        pane.cursor_row++;
        pane.modified = true;
        status_message = "Line duplicated";
    }
}

void ModernTextEditor::deleteCurrentLine()
{
    EditorPane &pane = panes[active_pane];
    if (pane.lines.size() > 1)
    {
        saveUndoState("delete line");
        pane.lines.erase(pane.lines.begin() + pane.cursor_row);
        if (pane.cursor_row >= static_cast<int>(pane.lines.size()))
            pane.cursor_row = static_cast<int>(pane.lines.size()) - 1;
        pane.cursor_col = 0;
        pane.modified = true;
        status_message = "Line deleted";
    }
}

void ModernTextEditor::selectCurrentLine()
{
    EditorPane &pane = panes[active_pane];
    pane.selection.active = true;
    pane.selection.start_row = pane.cursor_row;
    pane.selection.start_col = 0;
    pane.selection.end_row = pane.cursor_row;
    pane.selection.end_col = pane.lines[pane.cursor_row].length();
    status_message = "Line selected";
}

void ModernTextEditor::startGoToLine()
{
    // Go to line mode - kullanıcı satır numarası girecek
    mode = COMMAND_MODE;
    command_buffer = "goto ";
    status_message = "Go to line: ";
}

void ModernTextEditor::startQuickOpen()
{
    // Quick file open - basit file dialog
    openFile();
}

void ModernTextEditor::increaseFontSize()
{
    DeleteObject(hFont);

    // Font boyutunu artır (maksimum 24)
    if (current_font_size < 24)
        current_font_size += 2;

    hFont = CreateFont(current_font_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("0xNerd Proto"));

    calculateCharSize();
    updatePaneLayout();
    status_message = "Font size increased to " + std::to_string(current_font_size);
}

void ModernTextEditor::decreaseFontSize()
{
    DeleteObject(hFont);

    // Font boyutunu azalt (minimum 8)
    if (current_font_size > 8)
        current_font_size -= 2;

    hFont = CreateFont(current_font_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("0xNerd Proto"));

    calculateCharSize();
    updatePaneLayout();
    status_message = "Font size decreased to " + std::to_string(current_font_size);
}

void ModernTextEditor::resetFontSize()
{
    DeleteObject(hFont);

    // Font boyutunu sıfırla (varsayılan 16)
    current_font_size = 16;

    hFont = CreateFont(current_font_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, TEXT("0xNerd Proto"));

    calculateCharSize();
    updatePaneLayout();
    status_message = "Font size reset to " + std::to_string(current_font_size);
}

void ModernTextEditor::ensureCursorVisible()
{
    EditorPane &pane = panes[active_pane];

    // Pane boyutlarını hesapla
    int visible_lines = (pane.rect.bottom - pane.rect.top - 60) / (char_height + 2);

    // Cursor üstte görünmüyor - yukarı scroll
    if (pane.cursor_row < pane.scroll_top)
    {
        pane.scroll_top = pane.cursor_row;
    }
    // Cursor altta görünmüyor - aşağı scroll
    else if (pane.cursor_row >= pane.scroll_top + visible_lines)
    {
        pane.scroll_top = pane.cursor_row - visible_lines + 1;
    }

    // Scroll sınırlarını kontrol et
    if (pane.scroll_top < 0)
        pane.scroll_top = 0;

    int max_scroll = static_cast<int>(pane.lines.size()) - visible_lines;
    if (max_scroll < 0)
        max_scroll = 0;
    if (pane.scroll_top > max_scroll)
        pane.scroll_top = max_scroll;
}