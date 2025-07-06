#include <windows.h>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <cctype>
#include <richedit.h>
#include <commdlg.h>
#include <algorithm>

// Pencere sınıfı adı
const char *WINDOW_CLASS = "ModernTextEditor";

// Editör modları
enum EditorMode
{
    INSERT_MODE,
    COMMAND_MODE
};

// Seçim yapısı
struct Selection
{
    int start_row, start_col;
    int end_row, end_col;
    bool active;

    Selection() : start_row(0), start_col(0), end_row(0), end_col(0), active(false) {}

    void clear() { active = false; }

    bool hasSelection() const { return active && (start_row != end_row || start_col != end_col); }
};

// Editör penceresi yapısı
struct EditorPane
{
    std::vector<std::string> lines;
    int cursor_row;
    int cursor_col;
    int scroll_top;
    RECT rect;
    bool is_active;
    Selection selection;
    std::string filename;
    bool modified;

    EditorPane() : cursor_row(0), cursor_col(0), scroll_top(0), is_active(false), modified(false)
    {
        lines.push_back("");
        filename = "Untitled";
    }
};

// Ana editör sınıfı
class ModernTextEditor
{
private:
    HWND hwnd;
    EditorMode mode;
    std::vector<EditorPane> panes;
    int active_pane;
    std::string command_buffer;
    std::string status_message;
    std::string clipboard_data;

    // Font ayarları
    HFONT hFont;
    int char_width;
    int char_height;

    // Renkler
    HBRUSH bg_brush;
    HBRUSH status_brush;
    HBRUSH cursor_brush;
    HBRUSH selection_brush;

    // Shift tuşu durumu
    bool shift_pressed;
    bool ctrl_pressed;

public:
    ModernTextEditor() : mode(INSERT_MODE), active_pane(0), shift_pressed(false), ctrl_pressed(false)
    {
        // İlk pane'i oluştur
        panes.push_back(EditorPane());
        panes[0].is_active = true;

        status_message = "INSERT MODE - Ctrl+C: Copy, Ctrl+V: Paste, Ctrl+A: Select All";

        // Font oluştur
        hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

        // Renkler
        bg_brush = CreateSolidBrush(RGB(30, 30, 30));          // Koyu gri arka plan
        status_brush = CreateSolidBrush(RGB(0, 120, 215));     // Mavi status bar
        cursor_brush = CreateSolidBrush(RGB(255, 255, 255));   // Beyaz cursor
        selection_brush = CreateSolidBrush(RGB(70, 130, 180)); // Seçim rengi

        // Karakter boyutlarını hesapla
        calculateCharSize();
    }

    ~ModernTextEditor()
    {
        DeleteObject(hFont);
        DeleteObject(bg_brush);
        DeleteObject(status_brush);
        DeleteObject(cursor_brush);
        DeleteObject(selection_brush);
    }

    void setHwnd(HWND h) { hwnd = h; }

    void calculateCharSize()
    {
        HDC hdc = GetDC(NULL);
        SelectObject(hdc, hFont);

        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        char_width = tm.tmAveCharWidth;
        char_height = tm.tmHeight;

        ReleaseDC(NULL, hdc);
    }

    void updatePaneLayout()
    {
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        int pane_count = panes.size();
        int pane_width = client_rect.right / pane_count;
        int status_height = char_height + 10;

        for (int i = 0; i < pane_count; i++)
        {
            panes[i].rect.left = i * pane_width;
            panes[i].rect.right = (i + 1) * pane_width;
            panes[i].rect.top = 0;
            panes[i].rect.bottom = client_rect.bottom - status_height;
        }
    }

    void handleKeyPress(WPARAM wParam)
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
                // Undo işlemi için gelecekte implement edilebilir
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

    void handleInsertMode(WPARAM wParam)
    {
        EditorPane &pane = panes[active_pane];

        switch (wParam)
        {
        case VK_ESCAPE:
            mode = COMMAND_MODE;
            command_buffer.clear();
            pane.selection.clear();
            status_message = "COMMAND MODE - :help for commands";
            break;

        case VK_UP:
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();

            if (pane.cursor_row > 0)
            {
                pane.cursor_row--;
                if (pane.cursor_col > pane.lines[pane.cursor_row].length())
                    pane.cursor_col = pane.lines[pane.cursor_row].length();
            }
            break;

        case VK_DOWN:
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();

            if (pane.cursor_row < pane.lines.size() - 1)
            {
                pane.cursor_row++;
                if (pane.cursor_col > pane.lines[pane.cursor_row].length())
                    pane.cursor_col = pane.lines[pane.cursor_row].length();
            }
            break;

        case VK_LEFT:
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();

            if (pane.cursor_col > 0)
            {
                pane.cursor_col--;
            }
            else if (pane.cursor_row > 0)
            {
                pane.cursor_row--;
                pane.cursor_col = pane.lines[pane.cursor_row].length();
            }
            break;

        case VK_RIGHT:
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();

            if (pane.cursor_col < pane.lines[pane.cursor_row].length())
            {
                pane.cursor_col++;
            }
            else if (pane.cursor_row < pane.lines.size() - 1)
            {
                pane.cursor_row++;
                pane.cursor_col = 0;
            }
            break;

        case VK_HOME:
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();
            pane.cursor_col = 0;
            break;

        case VK_END:
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();
            pane.cursor_col = pane.lines[pane.cursor_row].length();
            break;

        case VK_PRIOR: // Page Up
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();
            pane.cursor_row = std::max(0, pane.cursor_row - 10);
            if (pane.cursor_col > pane.lines[pane.cursor_row].length())
                pane.cursor_col = pane.lines[pane.cursor_row].length();
            break;

        case VK_NEXT: // Page Down
            if (shift_pressed)
                updateSelection();
            else
                pane.selection.clear();
            pane.cursor_row = std::min((int)pane.lines.size() - 1, pane.cursor_row + 10);
            if (pane.cursor_col > pane.lines[pane.cursor_row].length())
                pane.cursor_col = pane.lines[pane.cursor_row].length();
            break;

        case VK_BACK:
            if (pane.selection.hasSelection())
            {
                deleteSelection();
            }
            else if (pane.cursor_col > 0)
            {
                pane.lines[pane.cursor_row].erase(pane.cursor_col - 1, 1);
                pane.cursor_col--;
                pane.modified = true;
            }
            else if (pane.cursor_row > 0)
            {
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
            else if (pane.cursor_col < pane.lines[pane.cursor_row].length())
            {
                pane.lines[pane.cursor_row].erase(pane.cursor_col, 1);
                pane.modified = true;
            }
            else if (pane.cursor_row < pane.lines.size() - 1)
            {
                pane.lines[pane.cursor_row] += pane.lines[pane.cursor_row + 1];
                pane.lines.erase(pane.lines.begin() + pane.cursor_row + 1);
                pane.modified = true;
            }
            break;

        case VK_RETURN:
            if (pane.selection.hasSelection())
                deleteSelection();

            {
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
        }
    }

    void handleCommandMode(WPARAM wParam)
    {
        if (wParam == VK_ESCAPE)
        {
            mode = INSERT_MODE;
            command_buffer.clear();
            status_message = "INSERT MODE - Ctrl+C: Copy, Ctrl+V: Paste, Ctrl+A: Select All";
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

    void handleChar(WPARAM wParam)
    {
        if (mode == INSERT_MODE && wParam >= 32 && wParam <= 126)
        {
            if (panes[active_pane].selection.hasSelection())
                deleteSelection();

            char ch = (char)wParam;
            insertText(std::string(1, ch));
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }

    void insertText(const std::string &text)
    {
        EditorPane &pane = panes[active_pane];
        pane.lines[pane.cursor_row].insert(pane.cursor_col, text);
        pane.cursor_col += text.length();
        pane.modified = true;
    }

    void updateSelection()
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

    void selectAll()
    {
        EditorPane &pane = panes[active_pane];
        pane.selection.active = true;
        pane.selection.start_row = 0;
        pane.selection.start_col = 0;
        pane.selection.end_row = pane.lines.size() - 1;
        pane.selection.end_col = pane.lines.back().length();

        status_message = "All text selected";
    }

    std::string getSelectedText()
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

    void copySelection()
    {
        std::string selected = getSelectedText();
        if (!selected.empty())
        {
            copyToClipboard(selected);
            status_message = "Copied to clipboard";
        }
    }

    void cutSelection()
    {
        std::string selected = getSelectedText();
        if (!selected.empty())
        {
            copyToClipboard(selected);
            deleteSelection();
            status_message = "Cut to clipboard";
        }
    }

    void deleteSelection()
    {
        EditorPane &pane = panes[active_pane];
        if (!pane.selection.hasSelection())
            return;

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
    }

    void copyToClipboard(const std::string &text)
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

    void pasteFromClipboard()
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

    void insertMultilineText(const std::string &text)
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

    void executeCommand()
    {
        if (command_buffer == "vsp" || command_buffer == "vsplit")
        {
            verticalSplit();
            status_message = "Vertical split created";
        }
        else if (command_buffer == "q" || command_buffer == "quit")
        {
            if (panes.size() > 1)
            {
                closePane();
                status_message = "Pane closed";
            }
            else
            {
                PostQuitMessage(0);
            }
        }
        else if (command_buffer == "w" || command_buffer == "write")
        {
            saveFile();
        }
        else if (command_buffer == "wq")
        {
            saveFile();
            if (panes.size() > 1)
                closePane();
            else
                PostQuitMessage(0);
        }
        else if (command_buffer == "help")
        {
            status_message = "Commands: :vsp (split), :w (save), :q (quit), :wq (save & quit)";
        }
        else
        {
            status_message = "Unknown command: " + command_buffer;
        }

        command_buffer.clear();
        mode = INSERT_MODE;
    }

    void verticalSplit()
    {
        EditorPane new_pane;
        new_pane.lines = panes[active_pane].lines;
        new_pane.filename = panes[active_pane].filename;
        panes.push_back(new_pane);
        updatePaneLayout();
    }

    void closePane()
    {
        if (panes.size() > 1)
        {
            panes.erase(panes.begin() + active_pane);
            if (active_pane >= panes.size())
                active_pane = panes.size() - 1;
            updatePanes();
            updatePaneLayout();
        }
    }

    void switchActivePane()
    {
        if (panes.size() > 1)
        {
            panes[active_pane].is_active = false;
            active_pane = (active_pane + 1) % panes.size();
            panes[active_pane].is_active = true;
            status_message = "Switched to pane " + std::to_string(active_pane + 1);
        }
    }

    void updatePanes()
    {
        for (int i = 0; i < panes.size(); i++)
        {
            panes[i].is_active = (i == active_pane);
        }
    }

    void newFile()
    {
        EditorPane new_pane;
        panes[active_pane] = new_pane;
        status_message = "New file created";
    }

    void openFile()
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

    void loadFile(const std::string &filename)
    {
        FILE *file = fopen(filename.c_str(), "r");
        if (file)
        {
            EditorPane &pane = panes[active_pane];
            pane.lines.clear();

            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), file))
            {
                std::string line = buffer;
                if (!line.empty() && line.back() == '\n')
                    line.pop_back();
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                pane.lines.push_back(line);
            }

            if (pane.lines.empty())
                pane.lines.push_back("");

            pane.filename = filename;
            pane.cursor_row = 0;
            pane.cursor_col = 0;
            pane.modified = false;

            fclose(file);
            status_message = "File loaded: " + filename;
        }
        else
        {
            status_message = "Error opening file: " + filename;
        }
    }

    void saveFile()
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

    void paint(HDC hdc)
    {
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        // Arka planı koyu gri yap
        FillRect(hdc, &client_rect, bg_brush);

        // Font seç
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkMode(hdc, TRANSPARENT);

        // Her pane'i çiz
        for (int i = 0; i < panes.size(); i++)
        {
            drawPane(hdc, panes[i], i);
        }

        // Status bar çiz
        drawStatusBar(hdc);
    }

    void drawPane(HDC hdc, const EditorPane &pane, int pane_index)
    {
        // Pane'in içini koyu gri yap
        FillRect(hdc, &pane.rect, bg_brush);

        // Pane sınırlarını çiz
        HPEN border_pen = CreatePen(PS_SOLID,
                                    pane.is_active ? 2 : 1,
                                    pane.is_active ? RGB(0, 120, 215) : RGB(100, 100, 100));
        HPEN old_pen = (HPEN)SelectObject(hdc, border_pen);

        Rectangle(hdc, pane.rect.left, pane.rect.top, pane.rect.right, pane.rect.bottom);

        // Pane başlığı
        SetTextColor(hdc, RGB(0, 120, 215));
        std::string title = pane.filename;
        if (pane.modified)
            title += " *";
        if (pane.is_active)
            title += " [ACTIVE]";

        TextOut(hdc, pane.rect.left + 5, pane.rect.top + 5, title.c_str(), title.length());

        // Metin içeriğini çiz
        int line_y = pane.rect.top + 30;
        int line_height = char_height + 2;

        for (int i = pane.scroll_top; i < pane.lines.size() && line_y < pane.rect.bottom - line_height; i++)
        {
            const std::string &line = pane.lines[i];

            // Satır numarası
            SetTextColor(hdc, RGB(100, 100, 100));
            std::string line_num = std::to_string(i + 1);
            while (line_num.length() < 4)
                line_num = " " + line_num;
            TextOut(hdc, pane.rect.left + 5, line_y, line_num.c_str(), line_num.length());

            // Seçimi çiz
            if (pane.selection.hasSelection())
            {
                drawSelection(hdc, pane, i, line_y);
            }

            // Metin
            SetTextColor(hdc, RGB(220, 220, 220));
            int text_x = pane.rect.left + 50;
            TextOut(hdc, text_x, line_y, line.c_str(), line.length());

            // Cursor çiz
            if (pane.is_active && i == pane.cursor_row)
            {
                int cursor_x = text_x + (pane.cursor_col * char_width);
                RECT cursor_rect = {cursor_x, line_y, cursor_x + 2, line_y + line_height};
                FillRect(hdc, &cursor_rect, cursor_brush);
            }

            line_y += line_height;
        }

        // Seçili kalemi geri yükle ve sil
        SelectObject(hdc, old_pen);
        DeleteObject(border_pen);
    }

    void drawSelection(HDC hdc, const EditorPane &pane, int current_line, int line_y)
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

    void drawStatusBar(HDC hdc)
    {
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        RECT status_rect = client_rect;
        status_rect.top = client_rect.bottom - (char_height + 10);

        FillRect(hdc, &status_rect, status_brush);

        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);

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

        TextOut(hdc, 10, status_rect.top + 5, left_status.c_str(), left_status.length());

        std::string mode_info = (mode == INSERT_MODE) ? "-- INSERT --" : "-- COMMAND --";
        SIZE text_size;
        GetTextExtentPoint32(hdc, mode_info.c_str(), mode_info.length(), &text_size);
        TextOut(hdc, client_rect.right - text_size.cx - 10, status_rect.top + 5,
                mode_info.c_str(), mode_info.length());
    }

    void handleResize()
    {
        updatePaneLayout();
        InvalidateRect(hwnd, NULL, FALSE);
    }
};

// Global editör instance
ModernTextEditor *g_editor = nullptr;

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        g_editor = new ModernTextEditor();
        g_editor->setHwnd(hwnd);
        g_editor->handleResize();
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        HDC mem_dc = CreateCompatibleDC(hdc);
        HBITMAP mem_bitmap = CreateCompatibleBitmap(hdc, client_rect.right, client_rect.bottom);
        SelectObject(mem_dc, mem_bitmap);

        if (g_editor)
        {
            g_editor->paint(mem_dc);
        }

        BitBlt(hdc, 0, 0, client_rect.right, client_rect.bottom, mem_dc, 0, 0, SRCCOPY);

        DeleteObject(mem_bitmap);
        DeleteDC(mem_dc);

        EndPaint(hwnd, &ps);
    }
    break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_KEYDOWN:
        if (g_editor)
        {
            g_editor->handleKeyPress(wParam);
        }
        break;

    case WM_CHAR:
        if (g_editor)
        {
            g_editor->handleChar(wParam);
        }
        break;

    case WM_SIZE:
        if (g_editor)
        {
            g_editor->handleResize();
        }
        break;

    case WM_DESTROY:
        delete g_editor;
        g_editor = nullptr;
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Window class kaydet
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = NULL;
    wc.lpszClassName = WINDOW_CLASS;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, "Window class registration failed!", "Error", MB_OK);
        return 1;
    }

    // Pencere oluştur
    HWND hwnd = CreateWindow(
        WINDOW_CLASS,
        "Modern Text Editor",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 800,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
    {
        MessageBox(NULL, "Window creation failed!", "Error", MB_OK);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Mesaj döngüsü
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}