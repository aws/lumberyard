/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <string.h>
#include <stdlib.h>
#include <ncursesw/form.h>
#include <ncursesw/menu.h>

#define STARTX 13
#define STARTY 4
#define WIDTH 20
#define N_FIELDS 5

#define MENU_ITEMS 5

#define MIN_ROW 20
#define MIN_COL 93
int main(int argc, char** argv)
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage %s [Condition] [File] [Line] [Reason]\n", argv[0]);
        return -1;
    }
    /* Resize Terminal */
    //  system("printf '\e[8;20;93t'");

    FIELD* field[N_FIELDS];
    FORM* my_form;
    WINDOW* menu_win;
    ITEM** menu_items;
    MENU* menu;


    int ch, i, result;

    /* Initialize curses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    const char title[] = "CryEngine Assert Dialog (NCURSES)";
    const char field1_title[] = "Condition:";
    const char field2_title[] = "File     :";
    const char field3_title[] = "Line     :";
    const char field4_title[] = "Reason   :";

    const char* menu_labels[] = {
        "Continue (c)",
        "Ignore (i)",
        "Ignore All (a)",
        "Break (b)",
        "Stop (s)"
    };

    /* Get Window Limits */
    int row, col;
    getmaxyx(stdscr, row, col);

    /* Create Form */

    /* Initialize the fields */
    /* Initialize the fields */
    for (i = 0; i < N_FIELDS - 1; ++i)
    {
        field[i] = new_field(1, col - STARTX - 2, STARTY + i * 2, STARTX, 0, 0);
        field_opts_off(field[i], O_ACTIVE); /* This field is a static label */
        //set_field_just(field[i], JUSTIFY_CENTER); /* Center Justification */
        set_field_buffer(field[i], 0, argv[i + 1]);
    }
    field[N_FIELDS - 1] = NULL;

    /* Set field options */


    /* Create the form and post it */
    my_form = new_form(field);
    post_form(my_form);
    refresh();


    attron(A_BOLD);
    /* Initialize the field  */
    mvprintw(STARTY, STARTX - 11, field1_title);
    mvprintw(STARTY + 2, STARTX - 11, field2_title);
    mvprintw(STARTY + 4, STARTX - 11, field3_title);
    mvprintw(STARTY + 6, STARTX - 11, field4_title);
    mvprintw(2, (col - (int)strlen(title)) / 2, title);
    mvprintw(STARTY + 9, 2, "Select an option:");
    attroff(A_BOLD);
    refresh();

    /* Create Menu  */
    menu_items = (ITEM**)calloc(MENU_ITEMS + 1, sizeof(ITEM*));
    for (i = 0; i < MENU_ITEMS; ++i)
    {
        menu_items[i] = new_item(menu_labels[i], menu_labels[i]);
    }
    menu_items[MENU_ITEMS] = NULL;


    menu_win = newwin(3, col - 2, STARTY + 10, 1);
    keypad(menu_win, TRUE);

    menu = new_menu((ITEM**)menu_items);
    keypad(menu_win, TRUE);
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, derwin(menu_win, 1, col - 4, 1, 1));
    menu_opts_off(menu, O_SHOWDESC);
    set_menu_format(menu, 1, 5);
    set_menu_mark(menu, "*c");
    box(menu_win, 0, 0);
    post_menu(menu);
    refresh();
    wrefresh(menu_win);
    ITEM* current;
    int exit = 0;
    /* Loop through to get user requests */
    while (!exit)
    {
        ch = wgetch(menu_win);
        switch (ch)
        {
        case KEY_LEFT:
            menu_driver(menu, REQ_LEFT_ITEM);
            break;
        case KEY_RIGHT:
            menu_driver(menu, REQ_RIGHT_ITEM);
            break;
        case 'c':
            result = 0;
            exit = 1;
            break;
        case 'i':
            result = 1;
            exit = 1;
            break;
        case 'a':
            result = 2;
            exit = 1;
            break;
        case 'b':
            result = 3;
            exit = 1;
            break;
        case 's':
            result = 4;
            exit = 1;
            break;
        case 10:
            current = current_item(menu);
            result = current->index;
            exit = 1;
        default:
            break;
        }
        wrefresh(menu_win);
    }

    /* Un post form and free the memory */
    unpost_form(my_form);
    free_form(my_form);
    for (i = 0; i < N_FIELDS - 1; ++i)
    {
        free_field(field[i]);
    }

    free_menu(menu);
    for (i = 0; i < MENU_ITEMS; ++i)
    {
        free_item(menu_items[i]);
    }
    endwin();
    return result;
}
