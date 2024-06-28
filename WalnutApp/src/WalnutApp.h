#pragma once
#include <iostream>
#include <sqlite3.h>
#include <sstream>
#include <chrono>
#include <fstream>

static bool load_open = true;
static bool load_closed = false;
static bool menu_open = false;
static bool admin_open = false;
static bool receipt_open = false;
static bool payment_open = false;
static bool order_open = false;
static bool history_open = false;

static bool animate = false;
static bool animate_gone = false;

static bool card = false;
static bool deletecard = false;
static bool cancelcard = true;
static bool deleteactive = true;
static bool addactive = true;
static bool can_order = false;
static bool Is_users = false;

static bool m_DrawOrderModal = false;

static double sum = 0;
static int count = 0;
const char* text = "Connecting to DataBase...";
const char* errortext = "";
static std::string selected = "0 American";
static std::string selected_data = "";
static std::string selected_phone = "";
static std::string selected_card = "";
static double selected_total;
static std::string selected_city = "";
static std::string selected_street = "";
static std::string selected_house = "";
static std::string selected_number = "";
static std::string queryResult = "";
static bool first = true;
const char* User = "123";
struct FoodType {
	const char* first;
	double firstcost;
	const char* second;
	double secondcost;
	const char* third;
	double thirdcost;
	const char* fourth;
	double fourthcost;
	const char* details;
};
std::vector<std::string> orderedFoods;
std::vector<double> orderedCost;
static std::map<std::string, int> foodCounts;
int number;
std::map<std::string, FoodType> foodTypes = {
	{"0 American", {"Burger",5.25f, "Fries",2.50f, "Chicken sandwich",4.99f, "HotDog",5.20f, "5"}},
	{"1 Asian", {"California roll",15.1f, "Unagi",13.2f, "Nigirizushi",8.f, "Makizushi",10.f, "Sushi"}},
	{"2 European", {"Minestrone",5.f, "Lentil Soup",4.f, "Chicken Tortilla Soup",6.f, "French Onion Soup",7.f, "Soups"}},
	{"3 Italian", {"Margherita",12.4f, "Lasagne alla Bolognese",15.f, "Gnocchi di Patate",10.f, "Fettuccine al Pomodoro",6.f, "Pizza"}},
	{"4 Mexican", {"Tacos",8.f, "Pozole",6.f, "Sopaipillas",10.f, "Enchiladas",9.f, "Taco"}},
	{"5 Vegetarian", {"Vegetarian Meatballs",10.3f, "Falafel",5.f, "Cauliflower Curry",7.5f, "Cauliflower Pizza Crust",10.f, "Vegetables"}},
	{"6 Salads", {"Greek salad",5.f, "Herring salad",5.f, "Insalata Caprese",5.f, "Chicken salad",5.f, "Salads"}},
	{"7 Sweets", {"Chocolate Donuts",7.5f, "Waffles & Maple syrop",10.5f, "Chocolate Croissant",10.f, "Apple pie",5.f, "Sweets"}},
	{"8 Ice Cream", {"Chocolate Ice Cream",1.75f, "Vanilla Ice Cream",1.25f, "Strawberry Ice Cream",1.5f, "Pistachio Ice Cream",2.3f, "Ice Creams IceCream"}},
	{"9 Drinks", {"Cola 0,5ml",3.00f, "Sprite 0,5ml",2.50f, "Black Tea",1.25f, "Coffe",2.00f, "Drinks"}},
};
struct HistoryData {
	std::string currentData;
	std::string name;
	std::string phoneNumber;
	std::string cardNumber;
	double total;
	std::string city;
	std::string street;
	std::string house;
	std::string number;
};
sqlite3* db;
int rc = sqlite3_open("Delivery.db", &db);

std::vector<HistoryData> historyDataVector;
