/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-02-28
    License: MIT
*/

#pragma once
#include <Stdinclude.hpp>
#include "Nuklear_GDI.hpp"
#include <shellapi.h>

using Consolecallback_t = void (__cdecl *)(int Argc, const wchar_t **Argv);

// Somewhat object-orientated.
template<size_t Maxtabs = 6>
struct Console_t
{
    // Extended log.
    bool isExtended{};

    // Console tabs.
    uint32_t Tabcount{};
    uint32_t Currenttab{ Maxtabs };
    std::array<std::string, Maxtabs> Titles{};
    std::array<std::string, Maxtabs> Filters{};
    std::array<uint32_t, Maxtabs> Unreadcounts{};
    std::deque<std::pair<std::string, struct nk_color>> Rawdata{};

    // Functionality.
    std::unordered_map<std::wstring, Consolecallback_t> Functions;

    // Window properties.
    struct nk_rect Region { 20, 50 };

    // Global input.
    std::array<char, 256> Inputstring;

    void Createtabs(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_style_item(Context, &Context->style.button.normal, nk_style_item_color(nk_rgb(0x32, 0x3A, 0x45)));
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x26, 0x23, 0x26)));

        // Tab-row is 25px high: [Static log] [Dynamic tabs] [Add new tab button]
        nk_layout_row_begin(Context, NK_STATIC, 25, 1 + Tabcount + !!(Tabcount < Maxtabs));
        {
            // Unfiltered log, and default tab.
            nk_layout_row_push(Context, 50);
            if (nk_button_label(Context, "LOG"))
                Currenttab = Maxtabs;

            // Dynamic tabs, unordered.
            for (uint32_t i = 0; i < Maxtabs; ++i)
            {
                // Uninitialized slot.
                if (Titles[i].empty()) continue;

                // Notification for new lines displayed in the tab.
                auto Title = Unreadcounts[i] == 0 ? ""s : va("[%-2d]", Unreadcounts[i]);
                Title += Titles[i];

                // Save the area before adding the button.
                nk_layout_row_push(Context, 100);
                const auto Area = nk_widget_bounds(Context);

                // Highlight the currently selected tab.
                if (i != Currenttab) Currenttab = nk_button_label(Context, Title.c_str()) ? i : Currenttab;
                else
                {
                    nk_style_push_style_item(Context, &Context->style.button.normal, Context->style.button.active);
                    Currenttab = nk_button_label(Context, Title.c_str()) ? i : Currenttab;
                    nk_style_pop_style_item(Context);
                }

                // Right-click on the tab.
                if (nk_contextual_begin(Context, 0, nk_vec2(100, 35 * 1 /* Dropdown count */), Area))
                {
                    nk_layout_row_dynamic(Context, 25, 1);
                    if (nk_contextual_item_label(Context, "Close", NK_TEXT_CENTERED))
                    {
                        Tabcount--;
                        Titles[i].clear();
                        if (i == Currenttab)
                            Currenttab = Maxtabs;
                    }

                    nk_contextual_end(Context);
                }
            }

            // Add tabs if we have unused slots.
            if (Tabcount < Maxtabs)
            {
                static char Input[2][64]; static int Length[2];
                const auto Area = nk_widget_bounds(Context);
                static bool Showpopup = false;

                // Fixed size.
                nk_layout_row_push(Context, 50);
                if (nk_button_symbol(Context, NK_SYMBOL_PLUS)) Showpopup = true;

                // Latched value.
                if (Showpopup)
                {
                    nk_style_push_vec2(Context, &Context->style.window.spacing, nk_vec2(0, 5));
                    if (nk_popup_begin(Context, NK_POPUP_DYNAMIC, "Add a new tab",
                                       NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE | NK_WINDOW_TITLE, nk_rect(Area.x - 20, 25, 300, 300)))
                    {
                        // Two widgets per row.
                        nk_layout_row_template_begin(Context, 25);
                        nk_layout_row_template_push_static(Context, 50);
                        nk_layout_row_template_push_dynamic(Context);
                        nk_layout_row_template_end(Context);
                        {
                            nk_label(Context, "Name:", NK_TEXT_LEFT);
                            nk_edit_string(Context, NK_EDIT_SIMPLE, Input[0], &Length[0], 64, nk_filter_default);

                            nk_label(Context, "Filter:", NK_TEXT_LEFT);
                            nk_edit_string(Context, NK_EDIT_SIMPLE, Input[1], &Length[1], 64, nk_filter_default);
                        }

                        // Full-row widget.
                        nk_layout_row_dynamic(Context, 25, 1);
                        if (Length[0])
                        {
                            if (nk_button_label(Context, "Submit"))
                            {
                                Input[0][Length[0]] = '\0';
                                Input[1][Length[1]] = '\0';
                                Length[0] = 0;
                                Length[1] = 0;

                                // Find an unused slot.
                                for (uint32_t i = 0; i < Maxtabs; ++i)
                                {
                                    // Already initialized slot.
                                    if (!Titles[i].empty()) continue;
                                    Filters[i] = Input[1];
                                    Titles[i] = Input[0];
                                    Unreadcounts[i] = 0;
                                    Tabcount++;
                                    break;
                                }

                                nk_popup_close(Context);
                                Showpopup = false;
                            }
                        }
                        else
                        {
                            // Inactive button.
                            const auto Button = Context->style.button;
                            Context->style.button.normal = nk_style_item_color(nk_rgb(40, 40, 40));
                            Context->style.button.active = nk_style_item_color(nk_rgb(40, 40, 40));
                            Context->style.button.hover = nk_style_item_color(nk_rgb(40, 40, 40));
                            Context->style.button.text_background = nk_rgb(60, 60, 60);
                            Context->style.button.border_color = nk_rgb(60, 60, 60);
                            Context->style.button.text_normal = nk_rgb(60, 60, 60);
                            Context->style.button.text_active = nk_rgb(60, 60, 60);
                            Context->style.button.text_hover = nk_rgb(60, 60, 60);
                            nk_button_label(Context, "Submit");
                            Context->style.button = Button;
                        }
                    }
                    else Showpopup = false;
                    nk_style_pop_vec2(Context);
                    nk_popup_end(Context);
                }
            }
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_style_item(Context);
        nk_style_pop_style_item(Context);
    }
    void Createlog(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_color(Context, &Context->style.window.background, nk_rgb(0x29, 0x26, 0x29));
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x29, 0x26, 0x29)));

        // Sized between 15% and 60% of the window.
        nk_layout_row_begin(Context, NK_STATIC, Region.h * (isExtended ? 0.6f : 0.15f), 1);
        {
            nk_layout_row_push(Context, Region.w);
            if (nk_group_begin(Context, "Console.Log", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
            {
                uint32_t Scrolloffset = 0;
                nk_layout_row_dynamic(Context, 20, 1);
                for (const auto &[String, Color] : Rawdata)
                {
                    // No filtering for maxtabs.
                    if(Currenttab == Maxtabs || std::strstr(String.c_str(), Filters[Currenttab].c_str()))
                    {
                        nk_label_colored(Context, String.c_str(), NK_TEXT_LEFT, Color);
                        Scrolloffset += 20;
                    }
                }

                const auto Y = std::max(0.0f, Scrolloffset - (Region.h * (isExtended ? 0.6f : 0.15f)) + 5);
                nk_group_set_scroll(Context, "Console.Log", 0, uint32_t(std::round(Y)));
                nk_group_end(Context);
            }
        }
        nk_layout_row_end(Context);

        // Restore the context.
        nk_style_pop_color(Context);
        nk_style_pop_style_item(Context);
    }
    void Createinput(nk_context *Context)
    {
        // Save the context properties.
        nk_style_push_style_item(Context, &Context->style.window.fixed_background, nk_style_item_color(nk_rgb(0x29, 0x26, 0x29)));

        nk_layout_row_dynamic(Context, 25, 1);
        {
            // Auto-focus.
            static int Inputlength;
            nk_edit_focus(Context, 0);
            auto Active = nk_edit_string(Context, NK_EDIT_SELECTABLE | NK_EDIT_FIELD | NK_EDIT_SIG_ENTER | NK_EDIT_GOTO_END_ON_ACTIVATE,
                                         Inputstring.data(), &Inputlength, Inputstring.size(), nk_filter_default);

            // On enter pressed.
            if (Active & NK_EDIT_COMMITED)
            {
                if (Inputlength)
                {
                    Inputstring.data()[Inputlength] = 0;
                    Inputlength = 0;

                    std::string ANSI(Inputstring.data());
                    std::wstring Wide(ANSI.begin(), ANSI.end());
                    Rawdata.push_back({ "> " + ANSI, nk_rgb(0xD6, 0xB7, 0x49) });

                    int Argc{}; const auto Argv{ CommandLineToArgvW(Wide.c_str(), &Argc) };
                    {
                        if (Argc >= 1)
                        {
                            if (auto Callback = Functions.find(Argv[0]); Callback != Functions.end())
                            {
                                Callback->second(Argc, (const wchar_t **)Argv);
                            }
                        }
                    }
                    LocalFree(Argv);


                    // TODO(tcn): Do something fun with the input.
                }
            }
        }

        // Restore the context.
        nk_style_pop_style_item(Context);
    }

    void onRender(nk_context *Context, struct nk_vec2 Windowsize)
    {
        Region.w = Windowsize.x - Region.x * 2;
        Region.h = Windowsize.y - Region.y;

        if (nk_begin(Context, "Console", Region, NK_WINDOW_NO_SCROLLBAR))
        {
            Context->style.window.spacing = nk_vec2(0, 0);
            Context->style.window.padding = nk_vec2(0, 0);
            Context->style.button.rounding = 0;

            Createtabs(Context);
            Createlog(Context);
            Createinput(Context);
        }
        nk_end(Context);
    }
};
