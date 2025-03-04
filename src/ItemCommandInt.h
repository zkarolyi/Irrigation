#pragma once

#include "BaseItemZeroWidget.h"

/**
 * @brief A menu item that executes a callback function when selected with an integer parameter.
 *
 * This class extends the BaseItemZeroWidget class and provides a menu item
 * that executes a callback function when selected. The callback function is
 * provided as a function pointer during construction.
 *
 * As a BaseItemZeroWidget, this item responds to selection events in the menu
 * system. When the user confirms the selection, handleCommit is triggered,
 * which executes the provided callback.
 */
class ItemCommandInt : public BaseItemZeroWidget
{
private:
    fptrInt callback = NULL;
    int parameter;

public:
    ItemCommandInt(const char *text, int param, fptrInt callback)
        : BaseItemZeroWidget(text), callback(callback), parameter(param) {}

protected:
    void handleCommit(LcdMenu *menu) override
    {
        if (callback)
            callback(parameter);
    }
};

/**
 * @brief Create a new command item.
 *
 * @param text The text to display for the item.
 * @param param The integer parameter to pass to the callback function.
 * @param callback The function to call when the item is selected.
 * @return MenuItem* The created item. Caller takes ownership of the returned pointer.
 *
 * @example
 *   auto item = ITEM_COMMAND_INT("Save", 3, [](int param) { save_data(param); });
 */

#define ITEM_COMMAND_INT(...) (new ItemCommandInt(__VA_ARGS__)) 
// inline MenuItem *ITEM_COMMAND_INT(const char *text, int param, fptrInt callback)
// {
//     return new ItemCommandInt(text, index, callback);
// }