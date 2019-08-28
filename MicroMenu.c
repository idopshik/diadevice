/**
              MICRO-MENU V2

          (C) Dean Camera, 2012
        www.fourwalledcubicle.com
     dean [at] fourwalledcubicle.com

        Royalty-free for all uses.
	                                  */

#include "MicroMenu.h"

/** This is used when an invalid menu handle is required in
 *  a \ref MENU_ITEM() definition, i.e. to indicate that a
 *  menu has no linked parent, child, next or previous entry.
 */
Menu_Item_t PROGMEM NULL_MENU = {0};

/** \internal
 *  Pointer to the generic menu text display function
 *  callback, to display the configured text of a menu item
 *  if no menu-specific display function has been set
 *  in the select menu item.
 */

// подтверждение о выборе меню можно задать двумя способами. Одни -generic через общую для всех функцию, считывающее поле TEXT
// из структуры. Или второй способ. Под каждую менюшку писать функцию и помещать её адресс в Select область структуры.
//Далее - generic указатель на (нужно написать)generic функцию.
static void (*MenuWriteFunc)(const char* Text) = NULL;

/** \internal
 *  Pointer to the currently selected menu item.
 */
static Menu_Item_t* CurrentMenuItem = &NULL_MENU; //изначально занулённый


Menu_Item_t* Menu_GetCurrentMenu(void)
{
	return CurrentMenuItem;
}

void Menu_Navigate(Menu_Item_t* const NewMenu)
{
	if ((NewMenu == &NULL_MENU) || (NewMenu == NULL))
		return;

	CurrentMenuItem = NewMenu;

	if (MenuWriteFunc) // Если такая функция есть, или на неё есть ненулевой указатель
		MenuWriteFunc(CurrentMenuItem->Text); // выводим на дисплей текст-название из структуры

	void (*SelectCallback)(void) = MENU_ITEM_READ_POINTER(&CurrentMenuItem->SelectCallback); // вызываем PGM_read и читаем указатель
	// Обращаемся к элементу структуры через указатель, поэтому -> Загружаем этот адресс тут же созданный указатель (он в структуре уже указатель
	// на функцию) В только что созданный указатель на void функцию. 

	if (SelectCallback) // Если указатель, считанный из структуры, не затычка нулевая
		SelectCallback(); // Вызываем функцию по выбору этого меню по этому указателю.
}
// Поместить её в Main{}; Она указывает компилятору на адресс. Её не надо вызывать много раз. 
void Menu_SetGenericWriteCallback(void (*WriteFunc)(const char* Text))  // Вызывается (нужно написать её) Функция, что пишет текст выбранного меню
// cчитанный из структуры.
{
	MenuWriteFunc = WriteFunc;
	Menu_Navigate(CurrentMenuItem);
}

void Menu_EnterCurrentItem(void)
{
	if ((CurrentMenuItem == &NULL_MENU) || (CurrentMenuItem == NULL)) // Если выбранное меню - нулевая затычка
		return;

	void (*EnterCallback)(void) = MENU_ITEM_READ_POINTER(&CurrentMenuItem->EnterCallback);// вызываем PGM_read и читаем указатель
	// Обращаемся к элементу структуры через указатель, поэтому -> Загружаем этот адресс тут же в созданный указатель (он в структуре уже указатель
	// на функцию) В только что созданный указатель на void функцию.

	if (EnterCallback)// Если указатель, считанный из структуры, не затычка нулевая
		EnterCallback();// Вызываем функцию по этому указателю.
}
