#ifndef MENU_H
#define MENU_H

#include <vector>
#include <string>

typedef struct MenuItem
{
    const struct MenuItem *subMenu; // Submenu pointer
    const int subMenuCount;         // submenu number
    const int boundaries;           // Menu boundaries
    std::vector<std::string> menuItemsText;
    void (*action)(void);           // Function pointer (optional)
} MenuItem;

MenuItem GetMenuItem(int L1, int L2, int L3);

#endif // MENU_H