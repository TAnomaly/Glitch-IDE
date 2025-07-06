#pragma once

#include <vector>
#include <string>
#include <windows.h>

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