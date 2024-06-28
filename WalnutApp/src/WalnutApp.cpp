#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"

#include "WalnutApp.h"

#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

static void ConnectToSQLServer() {

	char* errorMessage;
	std::cout << "SQLite created" << std::endl;


	//rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Clients (login TEXT PRIMARY KEY, password TEXT, number TEXT)", 0, 0, &errorMessage);
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Users (login TEXT PRIMARY KEY, password TEXT, number TEXT, admin BOOLEAN DEFAULT FALSE)", 0, 0, &errorMessage);
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Payment (client_login TEXT, cardnumber TEXT, ExpiryDate TEXT, CVC TEXT, FOREIGN KEY(client_login) REFERENCES Users(login))", 0, 0, &errorMessage);
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Deliver (client_login TEXT, city TEXT, street TEXT, house TEXT, number TEXT)", 0, 0, &errorMessage);
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS History (client_login TEXT, currentdata TEXT, phonenumber TEXT, cardnumber TEXT, total REAL, city TEXT, street TEXT, house TEXT, number TEXT)", 0, 0, &errorMessage);
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS FoodImages (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, chapter TEXT, image BLOB, cost REAL)", 0, 0, &errorMessage);


	if (rc == SQLITE_OK) {
		animate = true;
	}
}
class NewLayer : public Walnut::Layer
{
public:
	static void Calculate(const char* name, double cost) {
		if (foodCounts.find(name) != foodCounts.end()) {
			foodCounts[name]++;
			number = foodCounts[name]; // Set the number to the current count
		}
		else {
			foodCounts[name] = 1;
			orderedFoods.push_back(name);
			orderedCost.push_back(cost);
			number = 1; // Start the count for this new food item
		}
		sum += cost;
	}

	static void ShowLoadLayout(bool* p_open) {

		if (ImGui::Begin("Loading", p_open, ImGuiWindowFlags_NoCollapse)) {

			static float progress = 0.0f, progress_dir = 1.0f;
			float centerPos = (ImGui::GetWindowSize().x - 400.0f) * 0.50f;

			if (rc != SQLITE_OK) {
				text = "Failed to connect to DataBase...";
			}
			if (animate_gone) {

				centerPos = (ImGui::GetWindowSize().x - 300.0f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);
				centerPos = (ImGui::GetWindowSize().y - 150.0f) * 0.50f;
				ImGui::SetCursorPosY(centerPos);
				ImGui::Text("Welcome! Please log in or sign up.");
				centerPos = (ImGui::GetWindowSize().x - 250.0f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);
				const float inputWidth = 200.0f;
				ImGui::PushItemWidth(inputWidth);
				static char username[64] = "";
				static char password[64] = "";
				static char phone_number[64] = "";
				ImGui::InputTextWithHint("##name_input", "Username..", username, 32, ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::SameLine();
				ImGui::Text(errortext);
				ImGui::SetCursorPosX(centerPos);
				ImGui::InputTextWithHint("##firstpassword", "Password..", password, 32, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::SetCursorPosX(centerPos);
				const char* items[] = { "+1", "+44", "+52", "+91", "+86", "+48", "+380" };
				static int item_current = 0;
				ImGui::PushItemWidth(70.0f);
				ImGui::Combo("##cities", &item_current, items, IM_ARRAYSIZE(items));
				ImGui::PushItemWidth(125.0f);
				ImGui::SameLine();
				ImGui::InputTextWithHint("##phone_number", "Phone number..", phone_number, 32, ImGuiInputTextFlags_EnterReturnsTrue);
				const char* selected_item = items[item_current];
				char combined_text[128];
				snprintf(combined_text, sizeof(combined_text), "%s%s", selected_item, phone_number);
				ImGui::SetCursorPosX(centerPos);
				if (ImGui::Button(" Sign Up"))
				{
					rc = sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
					const char* checkQuery = "SELECT login FROM Users WHERE login = ?";
					sqlite3_stmt* checkStatement;
					rc = sqlite3_prepare_v2(db, checkQuery, -1, &checkStatement, 0);
					rc = sqlite3_bind_text(checkStatement, 1, username, -1, SQLITE_STATIC);
					if (sqlite3_step(checkStatement) == SQLITE_ROW) {
						errortext = "error, username already exists";
					}
					else {
						const char* insertQuery = "INSERT INTO Users VALUES (?, ?, ?, ?)";
						sqlite3_stmt* insertStatement;
						rc = sqlite3_prepare_v2(db, insertQuery, -1, &insertStatement, 0);
						rc = sqlite3_bind_text(insertStatement, 1, username, -1, SQLITE_STATIC);
						rc = sqlite3_bind_text(insertStatement, 2, password, -1, SQLITE_STATIC);
						rc = sqlite3_bind_text(insertStatement, 3, combined_text, -1, SQLITE_STATIC);
						int isAdmin = (strcmp(username, "admin") == 0) ? 1 : 0;
						rc = sqlite3_bind_int(insertStatement, 4, isAdmin);

						rc = sqlite3_step(insertStatement);

						if (rc == SQLITE_DONE) {
							rc = sqlite3_finalize(insertStatement);
							rc = sqlite3_exec(db, "COMMIT", 0, 0, 0);
							errortext = "Registration successfully";
						}
						else {
							rc = sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
							errortext = "error, failed to insert data";
						}
					}
					rc = sqlite3_finalize(checkStatement);
				}
				ImGui::SameLine();
				centerPos = (ImGui::GetWindowSize().x - 10.0f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);
				if (ImGui::Button("  Log In  "))
				{
					const char* checkQuery = "SELECT login, admin FROM Users WHERE login = ? AND password = ? AND number = ?";
					sqlite3_stmt* checkStatement;
					int rc;

					// Prepare the SQL statement
					rc = sqlite3_prepare_v2(db, checkQuery, -1, &checkStatement, 0);
					if (rc != SQLITE_OK) {
						errortext = "Failed to prepare statement";
						return;
					}

					// Bind the input parameters
					rc = sqlite3_bind_text(checkStatement, 1, username, -1, SQLITE_STATIC);
					if (rc != SQLITE_OK) {
						errortext = "Failed to bind username";
						sqlite3_finalize(checkStatement);
						return;
					}

					rc = sqlite3_bind_text(checkStatement, 2, password, -1, SQLITE_STATIC);
					if (rc != SQLITE_OK) {
						errortext = "Failed to bind password";
						sqlite3_finalize(checkStatement);
						return;
					}

					rc = sqlite3_bind_text(checkStatement, 3, combined_text, -1, SQLITE_STATIC);
					if (rc != SQLITE_OK) {
						errortext = "Failed to bind combined_text";
						sqlite3_finalize(checkStatement);
						return;
					}

					// Execute the query and check if a row is returned
					if (sqlite3_step(checkStatement) == SQLITE_ROW) {
						// Retrieve the isAdmin value from the query result
						int isAdmin = sqlite3_column_int(checkStatement, 1);

						// Set user and UI states
						User = username;
						load_open = false;


						if (isAdmin) {
							admin_open = true;
							ShowAdminLayout(&admin_open);
						}
						else {
							menu_open = true;
							ShowAppLayout(&menu_open);
						}

					}
					else {
						errortext = "Invalid login credentials";
					}

					// Finalize the SQL statement
					rc = sqlite3_finalize(checkStatement);
					if (rc != SQLITE_OK) {
						errortext = "Failed to finalize statement";
					}
				}

			}
			else {
				if (animate) {
					progress += progress_dir * 1.8f * ImGui::GetIO().DeltaTime;
					if (progress >= +1.1f) { progress = +1.1f; animate_gone = true; }
				}
				else {
					progress += progress_dir * 0.4f * ImGui::GetIO().DeltaTime;
					if (progress >= +0.6f) { progress = +0.6f; }

				}
				ImGui::SetCursorPosX(centerPos);
				centerPos = (ImGui::GetWindowSize().y - 150.0f) * 0.50f;
				ImGui::SetCursorPosY(centerPos);
				ImGui::ProgressBar(progress, ImVec2(400.0f, 0.0f));
				centerPos = (ImGui::GetWindowSize().x - 200.0f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);
				ImGui::Text(text);
				ConnectToSQLServer();
			}
			//sqlite3_close(db);
		}
		ImGui::End();
	}

	static void ShowAdminLayout(bool* p_open) {
		static bool is_users = false;
		static bool is_deliver = false;
		static bool is_history = false;
		static bool is_payment = false;
		static std::vector<std::vector<std::string>> queryResult;
		static char searchName[128] = "";  // Buffer for user name search input
		static char deleteName[128] = "";  // Buffer for user name delete input
		static char makeAdminName[128] = "";  // Buffer for user name to make admin input
		static char revokeAdminName[128] = ""; // Buffer for user name to revoke admin input
		static char imagePath[256] = "";   // Buffer for image file path input
		static char imageTitle[128] = "";  // Buffer for image title input
		static char imageChapter[128] = ""; // Buffer for image chapter input
		static char imageCost[64] = "";    // Buffer for image cost input
		static char deleteImageTitle[128] = ""; // Buffer for image title to delete input
		static char deleteImageChapter[128] = ""; // Buffer for image chapter to delete input
		static char manualQuery[1024] = "";

		if (ImGui::Begin("Admin panel", nullptr, ImGuiWindowFlags_NoTitleBar)) {
			ImGui::BeginChild("left pane", ImVec2(400, 0), true);
			ImGui::Text("Database Operations:");
			ImGui::Separator();

			// Define lambda functions for executing queries and updates
			auto executeQuery = [&](const char* query) {
				queryResult.clear();
				sqlite3_stmt* stmt = nullptr;
				int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
				if (rc == SQLITE_OK) {
					int columnCount = sqlite3_column_count(stmt);
					while (sqlite3_step(stmt) == SQLITE_ROW) {
						std::vector<std::string> row;
						for (int i = 0; i < columnCount; ++i) {
							const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
							row.push_back(text ? text : "NULL");
						}
						queryResult.push_back(row);
					}
				}
				else {
					queryResult.push_back({ "Failed to execute query: ", sqlite3_errmsg(db) });
				}
				if (stmt) {
					sqlite3_finalize(stmt);
				}
				};

			auto executeUpdate = [&](const char* query) {
				queryResult.clear();
				char* errMsg = nullptr;
				int rc = sqlite3_exec(db, query, nullptr, nullptr, &errMsg);
				if (rc != SQLITE_OK) {
					queryResult.push_back({ "Failed to execute update: ", errMsg });
					sqlite3_free(errMsg);
				}
				else {
					queryResult.push_back({ "Update executed successfully" });
				}
				};

			if (ImGui::Button("Get User Count")) {
				is_users = false;
				is_deliver = false;
				is_history = false;
				is_payment = false;
				executeQuery("SELECT COUNT(*) FROM Users");
			}

			if (ImGui::Button("Calculate Total Sum")) {
				is_users = false;
				is_deliver = false;
				is_history = false; // We set this to false as we are only calculating sum, not fetching history data
				is_payment = false;
				executeQuery("SELECT SUM(total) AS total_sum FROM History");
			}

			if (ImGui::Button("Get All Users")) {
				is_users = true;
				is_deliver = false;
				is_history = false;
				is_payment = false;
				executeQuery("SELECT * FROM Users");
			}

			if (ImGui::Button("Get Deliver Data")) {
				is_users = false;
				is_deliver = true;
				is_history = false;
				is_payment = false;
				executeQuery("SELECT * FROM Deliver");
			}

			if (ImGui::Button("Get History Data")) {
				is_users = false;
				is_deliver = false;
				is_history = true;
				is_payment = false;
				executeQuery("SELECT * FROM History");
			}

			if (ImGui::Button("Get Payment Data")) {
				is_users = false;
				is_deliver = false;
				is_history = false;
				is_payment = true;
				executeQuery("SELECT * FROM Payment");
			}

			ImGui::Separator();
			ImGui::Text("Find User by Name:");
			ImGui::InputText("User Name ", searchName, IM_ARRAYSIZE(searchName));
			if (ImGui::Button("Find User")) {
				is_users = true;
				is_deliver = false;
				is_history = false;
				is_payment = false;
				std::string query = "SELECT * FROM Users WHERE login = '" + std::string(searchName) + "'";
				executeQuery(query.c_str());
			}

			ImGui::Separator();
			ImGui::Text("Delete User by Name:");
			ImGui::InputText("User Name  ", deleteName, IM_ARRAYSIZE(deleteName));
			if (ImGui::Button("Delete User")) {
				is_users = false;
				is_deliver = false;
				is_history = false;
				is_payment = false;
				std::string query = "DELETE FROM Users WHERE login = '" + std::string(deleteName) + "'";
				executeUpdate(query.c_str());
				// Optionally re-fetch all users to update the list after deletion
				executeQuery("SELECT * FROM Users");
			}

			ImGui::Separator();
			ImGui::Text("Make User Admin by Name:");
			ImGui::InputText("User Name   ", makeAdminName, IM_ARRAYSIZE(makeAdminName));
			if (ImGui::Button("Make Admin")) {
				is_users = true;
				is_deliver = false;
				is_history = false;
				is_payment = false;
				std::string query = "UPDATE Users SET admin = 1 WHERE login = '" + std::string(makeAdminName) + "'";
				executeUpdate(query.c_str());
				// Optionally re-fetch all users to update the list after making admin
				executeQuery("SELECT * FROM Users");
			}

			ImGui::Text("Revoke Admin by Name:");
			ImGui::InputText("User Name", revokeAdminName, IM_ARRAYSIZE(revokeAdminName));
			if (ImGui::Button("Revoke Admin")) {
				is_users = true;
				is_deliver = false;
				is_history = false;
				is_payment = false;
				std::string query = "UPDATE Users SET admin = 0 WHERE login = '" + std::string(revokeAdminName) + "'";
				executeUpdate(query.c_str());
				// Optionally re-fetch all users to update the list after revoking admin
				executeQuery("SELECT * FROM Users");
			}
			ImGui::Separator();
			ImGui::Text("Manual SQL Query:");
			ImGui::InputTextMultiline("SQL Query", manualQuery, IM_ARRAYSIZE(manualQuery));
			if (ImGui::Button("Execute Manual Query")) {
				executeQuery(manualQuery);  // Using the existing query execution lambda
			}

			// Optionally, add a checkbox to toggle between query and update modes
			static bool isQuery = true;
			ImGui::Checkbox("Is Query", &isQuery);
			ImGui::SameLine();
			if (ImGui::Button("Execute")) {
				if (isQuery) {
					executeQuery(manualQuery);
				}
				else {
					executeUpdate(manualQuery);
				}
			}

			ImGui::Separator();
			ImGui::Text("Insert Food Image:");
			ImGui::InputText("Image Title", imageTitle, IM_ARRAYSIZE(imageTitle));
			ImGui::InputText("Image Chapter", imageChapter, IM_ARRAYSIZE(imageChapter));
			ImGui::InputText("Image Path", imagePath, IM_ARRAYSIZE(imagePath));
			ImGui::InputText("Image Cost", imageCost, IM_ARRAYSIZE(imageCost));

			if (ImGui::Button("Insert Image")) {
				queryResult.clear();  // Clear previous results
				std::ifstream file(imagePath, std::ios::binary);
				if (file) {
					std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
					std::string title(imageTitle);
					std::string chapter(imageChapter);
					double cost = std::stod(imageCost); // Convert cost to double
					sqlite3_stmt* stmt;
					const char* sql = "INSERT INTO FoodImages (title, chapter, image, cost) VALUES (?, ?, ?, ?)";
					int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
					if (rc == SQLITE_OK) {
						sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
						sqlite3_bind_text(stmt, 2, chapter.c_str(), -1, SQLITE_STATIC);
						sqlite3_bind_blob(stmt, 3, buffer.data(), static_cast<int>(buffer.size()), SQLITE_STATIC);
						sqlite3_bind_double(stmt, 4, cost); // Bind cost
						if (sqlite3_step(stmt) == SQLITE_DONE) {
							queryResult.push_back({ "Image inserted successfully" });
						}
						else {
							queryResult.push_back({ "Failed to insert image: ", sqlite3_errmsg(db) });
						}
					}
					else {
						queryResult.push_back({ "Failed to prepare statement: ", sqlite3_errmsg(db) });
					}
					sqlite3_finalize(stmt);
				}
				else {
					queryResult.push_back({ "Failed to open image file" });
				}
			}

			ImGui::Separator();
			ImGui::Text("Delete Food Image:");
			ImGui::InputText("Image Title ", deleteImageTitle, IM_ARRAYSIZE(deleteImageTitle));
			ImGui::InputText("Image Chapter ", deleteImageChapter, IM_ARRAYSIZE(deleteImageChapter));
			if (ImGui::Button("Delete Image")) {
				is_users = false;
				is_deliver = false;
				is_history = false;
				is_payment = false;
				std::string query = "DELETE FROM FoodImages WHERE title = '" + std::string(deleteImageTitle) + "' AND chapter = '" + std::string(deleteImageChapter) + "'";
				executeUpdate(query.c_str());
				// Optionally re-fetch all images to update the list after deletion
				executeQuery("SELECT * FROM FoodImages");
			}

			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("right pane", ImVec2(0, 0), true);
			ImGui::Text("Query Result:");
			ImGui::Separator();

			if (!queryResult.empty()) {
				int columnCount = queryResult[0].size();
				ImGui::BeginTable("QueryTable", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable);

				// Setup table headers based on the table being queried
				if (is_users) {
					ImGui::TableSetupColumn("Login", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Password", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Phone Number", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Admin False(0), True(1)", ImGuiTableColumnFlags_None);
					ImGui::TableHeadersRow();
				}
				else if (is_deliver) {
					ImGui::TableSetupColumn("Client Login", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("City", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Street", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("House", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Number", ImGuiTableColumnFlags_None);
					ImGui::TableHeadersRow();
				}
				else if (is_history) {
					ImGui::TableSetupColumn("Client Login", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Current Data", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Phone Number", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Card Number", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("City", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Street", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("House", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Number", ImGuiTableColumnFlags_None);
					ImGui::TableHeadersRow();
				}
				else if (is_payment) {
					ImGui::TableSetupColumn("Client Login", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Card Number", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("Expiry Date", ImGuiTableColumnFlags_None);
					ImGui::TableSetupColumn("CVC", ImGuiTableColumnFlags_None);
					ImGui::TableHeadersRow();
				}

				// Setup table rows
				for (const auto& row : queryResult) {
					ImGui::TableNextRow();
					for (const auto& cell : row) {
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(cell.c_str());
					}
				}

				ImGui::EndTable();
			}
			else {
				ImGui::TextWrapped("No results to display.");
			}

			ImGui::EndChild();
		}
		ImGui::End();
	}
	static void ShowAppLayout(bool* p_open) {
		static std::string selectedChapter;
		static std::vector<std::string> chapters;
		static std::unordered_map<int, std::shared_ptr<Walnut::Image>> imageCache;
		static std::vector<std::tuple<int, std::string, std::string, double>> items;

		// Fetch chapters from the database
		if (db != nullptr && chapters.empty()) {
			const char* sql_chapters = "SELECT DISTINCT chapter FROM FoodImages";
			sqlite3_stmt* stmt_chapters;
			int rc = sqlite3_prepare_v2(db, sql_chapters, -1, &stmt_chapters, nullptr);
			if (rc == SQLITE_OK) {
				while (sqlite3_step(stmt_chapters) == SQLITE_ROW) {
					std::string chapter(reinterpret_cast<const char*>(sqlite3_column_text(stmt_chapters, 0)));
					chapters.push_back(chapter);
				}
				sqlite3_finalize(stmt_chapters);
			}
			else {
				const char* errMsg = sqlite3_errmsg(db);
				ImGui::Text("Failed to prepare SQL statement for chapters: %s", errMsg);
				std::cerr << "Failed to prepare SQL statement for chapters: " << errMsg << std::endl;
			}
		}

		if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoTitleBar)) {
			ImGui::BeginChild("left pane", ImVec2(300, 0), true);
			ImGui::Text("Ordering:");
			ImGui::Separator();
			if (selectedChapter == "") {
				if (!chapters.empty()) {
					selectedChapter = chapters[0];
					// Invalidate cache and items when chapter is selected for the first time
					imageCache.clear();
					items.clear();
				}
			}

			for (const auto& chapter : chapters) {
				char label[128];
				snprintf(label, sizeof(label), "%s", chapter.c_str());
				if (ImGui::Selectable(label, selectedChapter == chapter)) {
					selectedChapter = chapter;
					// Invalidate cache and items when chapter is changed
					imageCache.clear();
					items.clear();
				}
			}
			ImGui::EndChild();

			if (!selectedChapter.empty()) {
				ImGui::SameLine();

				ImGui::BeginGroup();
				ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

				if (items.empty() && db != nullptr) {
					// Fetch items based on selected chapter
					const char* sql_items = "SELECT id, title, chapter, cost FROM FoodImages WHERE chapter = ?";
					sqlite3_stmt* stmt_items;
					int rc = sqlite3_prepare_v2(db, sql_items, -1, &stmt_items, nullptr);
					if (rc == SQLITE_OK) {
						sqlite3_bind_text(stmt_items, 1, selectedChapter.c_str(), -1, SQLITE_STATIC);
						while (sqlite3_step(stmt_items) == SQLITE_ROW) {
							int imageId = sqlite3_column_int(stmt_items, 0);
							std::string title(reinterpret_cast<const char*>(sqlite3_column_text(stmt_items, 1)));
							std::string chapter(reinterpret_cast<const char*>(sqlite3_column_text(stmt_items, 2)));
							double cost = sqlite3_column_double(stmt_items, 3);

							items.emplace_back(imageId, title, chapter, cost);
						}
						sqlite3_finalize(stmt_items);
					}
					else {
						const char* errMsg = sqlite3_errmsg(db);
						ImGui::Text("Failed to prepare SQL statement for items: %s", errMsg);
						std::cerr << "Failed to prepare SQL statement for items: " << errMsg << std::endl;
					}
				}

				if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
					if (ImGui::BeginTabItem("Description")) {
						ImGui::Separator();
						ImGui::Columns(2, "columns");

						int id = 0;
						for (const auto& item : items) {
							const auto& [imageId, title, chapter, cost] = item;

							std::shared_ptr<Walnut::Image> image;
							if (imageCache.find(imageId) == imageCache.end() && db != nullptr) {
								// Fetch image if not cached
								const char* sql_image = "SELECT image FROM FoodImages WHERE id = ?";
								sqlite3_stmt* stmt_image;
								int rc = sqlite3_prepare_v2(db, sql_image, -1, &stmt_image, nullptr);
								if (rc == SQLITE_OK) {
									sqlite3_bind_int(stmt_image, 1, imageId);
									if (sqlite3_step(stmt_image) == SQLITE_ROW) {
										const void* blob = sqlite3_column_blob(stmt_image, 0);
										int blobSize = sqlite3_column_bytes(stmt_image, 0);
										Walnut::Application::Get().AddImageToVector(blob, blobSize, image);
										imageCache[imageId] = image;
									}
									sqlite3_finalize(stmt_image);
								}
								else {
									const char* errMsg = sqlite3_errmsg(db);
									ImGui::Text("Failed to prepare SQL statement for image: %s", errMsg);
									std::cerr << "Failed to prepare SQL statement for image: " << errMsg << std::endl;
								}
							}
							else {
								// Use cached image
								image = imageCache[imageId];
							}

							// Display image and metadata
							ImGui::BeginGroup();
							if (image) {
								ImGui::Image(image->GetDescriptorSet(), ImVec2(148, 148));
							}
							else {
								ImGui::Text("Image not available");
							}
							ImGui::TextWrapped("%s", title.c_str());
							ImGui::Text("%g $", cost);
							ImGui::PushID(id++);
							if (ImGui::Button("Order", ImVec2(140, 40))) {
								receipt_open = true;
								if (!selectedChapter.empty()) {
									Calculate(title.c_str(), cost);
								}
							}
							ImGui::PopID();
							ImGui::EndGroup();
							ImGui::NextColumn();
						}

						ImGui::EndTabItem();
						ImGui::Separator();
					}

					ImGui::EndTabBar();
				}
				ImGui::EndChild();
				ImGui::EndGroup();
			}
		}
		ImGui::End();
	}
	static bool IsValidCardNumber(const char* cardnumber) {
		if (cardnumber[0] == '2' || cardnumber[0] == '4' || cardnumber[0] == '5' || cardnumber[0] == '3') {
			int length = 0;
			for (int i = 0; cardnumber[i]; i++) {
				if (cardnumber[i] != ' ') {
					length++;
					if (!isdigit(cardnumber[i])) {
						return false;
					}
				}
			}
			return length == 16;
		}
		return false;
	}
	static bool IsValidExpiryDate(const char* expiryDate) {
		int month, year;
		if (sscanf(expiryDate, "%d/%d", &month, &year) != 2) {
			return false; // Invalid format
		}
		// Check if the month and year are within valid ranges
		return (month >= 1 && month <= 12) && (year >= 0 && year <= 99);
	}
	static bool IsValid(const char* street, const char* house, const char* number) {
		return (street && house && number && street[0] && house[0] && number[0]);
	}
	static void HelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	static void ShowOrder(bool* p_open) {
		if (ImGui::Begin("Order", p_open, ImGuiWindowFlags_NoTitleBar)) {
			static char street[64] = "";
			static char house[6] = "";
			static char number[6] = "";
			static char phonenumber[20] = "";
			static char cardnumber[20] = "";
			static const char* items[] = { "San Francisco", "Oakland", "San Jose", "Berkeley", "Palo Alto", "San Mateo", "Redwood City", "Daly City", "Mountain View", "Santa Clara", "Fremont" };
			static int item_current = 0;

			static bool is_data_fetched = false;

			if (!is_data_fetched) {
				// Fetch phone number from the database
				const char* selectPhoneQuery = "SELECT number FROM Users WHERE login = ?";
				sqlite3_stmt* selectPhoneStatement;
				int rc = sqlite3_prepare_v2(db, selectPhoneQuery, -1, &selectPhoneStatement, 0);
				if (rc == SQLITE_OK) {
					rc = sqlite3_bind_text(selectPhoneStatement, 1, User, -1, SQLITE_STATIC);
					if (rc == SQLITE_OK && sqlite3_step(selectPhoneStatement) == SQLITE_ROW) {
						const char* phoneFromDB = (const char*)sqlite3_column_text(selectPhoneStatement, 0);
						strncpy(phonenumber, phoneFromDB, sizeof(phonenumber) - 1);
						phonenumber[sizeof(phonenumber) - 1] = '\0';
					}
					sqlite3_finalize(selectPhoneStatement);
				}

				// Fetch card number from the database
				const char* selectCardQuery = "SELECT cardnumber FROM Payment WHERE client_login = ?";
				sqlite3_stmt* selectCardStatement;
				rc = sqlite3_prepare_v2(db, selectCardQuery, -1, &selectCardStatement, 0);
				if (rc == SQLITE_OK) {
					rc = sqlite3_bind_text(selectCardStatement, 1, User, -1, SQLITE_STATIC);
					if (rc == SQLITE_OK && sqlite3_step(selectCardStatement) == SQLITE_ROW) {
						const char* cardFromDB = (const char*)sqlite3_column_text(selectCardStatement, 0);
						strncpy(cardnumber, cardFromDB, sizeof(cardnumber) - 1);
						cardnumber[sizeof(cardnumber) - 1] = '\0';
					}
					sqlite3_finalize(selectCardStatement);
				}

				is_data_fetched = true;
			}

			ImGui::BeginChild("Delivery", ImVec2(1060.0f, 400.0f));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.1f));
			ImGui::BeginChild("Address", ImVec2(400.0f, 400.0f), true);

			ImGui::Text("Give your address");
			ImGui::Separator();

			ImGui::Columns(2, "1");
			ImGui::SetColumnWidth(0, 60.0f);
			ImGui::Text("City");
			ImGui::SameLine(); ImGui::NextColumn();
			ImGui::PushItemWidth(290.0f);
			ImGui::Combo("##cities", &item_current, items, IM_ARRAYSIZE(items));
			ImGui::SameLine(); HelpMarker("Ensure your address is correct");

			ImGui::NextColumn();
			ImGui::Text("Street");
			ImGui::SameLine(); ImGui::NextColumn();
			ImGui::PushItemWidth(290.0f);
			ImGui::InputTextWithHint("##street_input", "Mission St", street, 64, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AlwaysOverwrite);

			ImGui::NextColumn();
			ImGui::Text("House");
			ImGui::SameLine(); ImGui::NextColumn();
			ImGui::PushItemWidth(100.0f);
			ImGui::InputTextWithHint("##house", "201", house, 6, ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			ImGui::InputTextWithHint("##number", "1", number, 6, ImGuiInputTextFlags_EnterReturnsTrue);

			ImGui::EndColumns();
			ImGui::Separator();
			ImGui::Spacing();

			// Get current date and time
			std::time_t now = std::time(nullptr);
			std::tm tm = *std::localtime(&now);
			char currentData[64];
			std::strftime(currentData, sizeof(currentData), "%Y-%m-%d %H:%M:%S", &tm);

			// Button to place the order
			if (ImGui::Button("Deliver", ImVec2(100.0f, 30.0f))) {

				if (IsValid(street, house, number)) {
					ShowDrawModal();
					sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0); // Start transaction

					// Check if the address already exists in the Deliver table
					const char* checkQuery = "SELECT COUNT(*) AS count FROM Deliver WHERE client_login = ? AND city = ? AND street = ? AND house = ? AND number = ?";
					sqlite3_stmt* checkStatement;
					rc = sqlite3_prepare_v2(db, checkQuery, -1, &checkStatement, 0);
					if (rc == SQLITE_OK) {
						sqlite3_bind_text(checkStatement, 1, User, -1, SQLITE_STATIC);
						sqlite3_bind_text(checkStatement, 2, items[item_current], -1, SQLITE_STATIC);
						sqlite3_bind_text(checkStatement, 3, street, -1, SQLITE_STATIC);
						sqlite3_bind_text(checkStatement, 4, house, -1, SQLITE_STATIC);
						sqlite3_bind_text(checkStatement, 5, number, -1, SQLITE_STATIC);

						if (sqlite3_step(checkStatement) == SQLITE_ROW) {
							int count = sqlite3_column_int(checkStatement, 0);
							if (count == 0) {
								// Insert new delivery address
								const char* insertDeliverQuery = "INSERT INTO Deliver (client_login, city, street, house, number) VALUES (?, ?, ?, ?, ?)";
								sqlite3_stmt* insertStatement;
								rc = sqlite3_prepare_v2(db, insertDeliverQuery, -1, &insertStatement, 0);
								if (rc == SQLITE_OK) {
									sqlite3_bind_text(insertStatement, 1, User, -1, SQLITE_STATIC);
									sqlite3_bind_text(insertStatement, 2, items[item_current], -1, SQLITE_STATIC);
									sqlite3_bind_text(insertStatement, 3, street, -1, SQLITE_STATIC);
									sqlite3_bind_text(insertStatement, 4, house, -1, SQLITE_STATIC);
									sqlite3_bind_text(insertStatement, 5, number, -1, SQLITE_STATIC);

									if (sqlite3_step(insertStatement) == SQLITE_DONE) {
										sqlite3_finalize(insertStatement);
									}
									else {
										sqlite3_finalize(insertStatement);
										sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
										return;
									}
								}
							}
						}
						sqlite3_finalize(checkStatement);
					}

					// Insert order into history
					const char* insertHistoryQuery = "INSERT INTO History (client_login, currentdata, phonenumber, cardnumber, total, city, street, house, number) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
					sqlite3_stmt* insertHistoryStatement;
					rc = sqlite3_prepare_v2(db, insertHistoryQuery, -1, &insertHistoryStatement, 0);
					if (rc == SQLITE_OK) {
						sqlite3_bind_text(insertHistoryStatement, 1, User, -1, SQLITE_STATIC);
						sqlite3_bind_text(insertHistoryStatement, 2, currentData, -1, SQLITE_STATIC);
						sqlite3_bind_text(insertHistoryStatement, 3, phonenumber, -1, SQLITE_STATIC);
						sqlite3_bind_text(insertHistoryStatement, 4, cardnumber, -1, SQLITE_STATIC);
						sqlite3_bind_double(insertHistoryStatement, 5, sum);
						sqlite3_bind_text(insertHistoryStatement, 6, items[item_current], -1, SQLITE_STATIC);
						sqlite3_bind_text(insertHistoryStatement, 7, street, -1, SQLITE_STATIC);
						sqlite3_bind_text(insertHistoryStatement, 8, house, -1, SQLITE_STATIC);
						sqlite3_bind_text(insertHistoryStatement, 9, number, -1, SQLITE_STATIC);

						if (sqlite3_step(insertHistoryStatement) == SQLITE_DONE) {
							sqlite3_finalize(insertHistoryStatement);
							sqlite3_exec(db, "COMMIT", 0, 0, 0); // Commit transaction
						}
						else {
							sqlite3_finalize(insertHistoryStatement);
							sqlite3_exec(db, "ROLLBACK", 0, 0, 0); // Rollback transaction
						}
					}

					memset(street, 0, sizeof(street));
					memset(house, 0, sizeof(house));
					memset(number, 0, sizeof(number));
				}
				else {
					ImGui::Text("Invalid address or unable to place order.");
				}
			}

			ImGui::PopStyleColor();
			ImGui::EndChild();
			ImGui::EndChild();
		}

		ImGui::End();
	}
	static void ShowHistory(bool* p_open) {
		if (ImGui::Begin("History", p_open, ImGuiWindowFlags_NoTitleBar)) {
			const char* selectQuery = "SELECT client_login, currentdata, phonenumber, cardnumber, total, city, street, house, number FROM History WHERE client_login = ?";
			sqlite3_stmt* selectStatement;
			rc = sqlite3_prepare_v2(db, selectQuery, -1, &selectStatement, 0);
			rc = sqlite3_bind_text(selectStatement, 1, User, -1, SQLITE_STATIC);

			// Clear the vector before populating it with new data
			historyDataVector.clear();

			if (rc == SQLITE_OK) {
				while (sqlite3_step(selectStatement) == SQLITE_ROW) {
					HistoryData data;
					data.name = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 0));
					data.currentData = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 1));

					data.phoneNumber = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 2));
					data.cardNumber = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 3));
					data.total = sqlite3_column_double(selectStatement, 4);
					data.city = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 5));
					data.street = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 6));
					data.house = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 7));
					data.number = reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, 8));

					historyDataVector.push_back(data);
				}

				if (historyDataVector.empty()) {
					ImGui::Text("No data found for the selected name.");
				}

				sqlite3_finalize(selectStatement);
			}
			else {
				ImGui::Text("Error executing the SELECT query.");
			}

			ImGui::SameLine();
			ImGui::BeginChild("Client History", ImVec2(655.0f, 400.0f));
			ImGui::Text("Client History:");
			ImGui::Separator();
			ImGui::Columns(2);

			if (!historyDataVector.empty()) {
				for (const auto& currentData : historyDataVector) {
					std::string label = currentData.currentData;
					if (ImGui::Selectable(label.c_str(), selected_data == currentData.currentData)) {
						selected_data = currentData.currentData;
						selected_phone = currentData.phoneNumber;
						selected_card = currentData.cardNumber;
						selected_total = currentData.total;
						selected_number = currentData.number;
						selected_house = currentData.house;
						selected_street = currentData.street;
						selected_city = currentData.city;
					}
				}
			}

			ImGui::NextColumn();
			ImGui::SetColumnWidth(0, 200.0f);
			ImGui::BeginGroup();
			ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
			float old_size = ImGui::GetFont()->Scale;
			ImGui::GetFont()->Scale -= 0.2f;
			ImGui::PushFont(ImGui::GetFont());
			ImGui::TextColored(ImVec4(0.64f, 0.04f, 0.15f, 1.0f), "24 Food Delivery");

			ImGui::Text("Address: %s %s %s %s", selected_number.c_str(), selected_house.c_str(), selected_street.c_str(), selected_city.c_str());
			ImGui::GetFont()->Scale = old_size;
			ImGui::PopFont();
			ImGui::Text("Card: %s", selected_card.c_str());
			ImGui::Text("Phone: %s", selected_phone.c_str());
			ImGui::TextColored(ImVec4(0.341f, 0.51f, 0.17f, 1.0f), "Total: %g$", selected_total);

			std::chrono::system_clock::time_point selected_time;
			{
				std::istringstream ss(selected_data);
				std::tm selected_tm = {};
				ss >> std::get_time(&selected_tm, "%Y-%m-%d %H:%M:%S");
				selected_time = std::chrono::system_clock::from_time_t(std::mktime(&selected_tm));
			}
			selected_time += std::chrono::minutes(20);
			auto current_time = std::chrono::system_clock::now();
			std::string status;
			if (selected_time < current_time) {
				status = "Delivered";
			}
			else {
				status = "In Progress";
			}
			std::time_t selected_time_t_result = std::chrono::system_clock::to_time_t(selected_time);
			std::tm selected_tm_result = *std::localtime(&selected_time_t_result);
			char selected_data_result[64];
			std::strftime(selected_data_result, sizeof(selected_data_result), "%Y-%m-%d %H:%M:%S", &selected_tm_result);

			ImGui::Text("Ordered in: %s", selected_data.c_str());
			ImGui::Text("Delivered in: %s", selected_data_result);
			ImGui::TextColored(ImVec4(0.50f, 0.45f, 0.83f, 1.0f), "Status: %s", status.c_str());

			ImGui::EndChild();
			ImGui::EndGroup();

			ImGui::EndColumns();
			ImGui::EndChild();
		}
		ImGui::End();
	}

	static void ShowPayment(bool* p_open) {
		if (ImGui::Begin("Payment", p_open, ImGuiWindowFlags_NoTitleBar))
		{
			float old_size = ImGui::GetFont()->Scale;
			ImGui::GetFont()->Scale += 0.2f;
			ImGui::PushFont(ImGui::GetFont());
			float centerPos = (ImGui::GetWindowSize().x - 100.f) * 0.50f;
			ImGui::SetCursorPosX(centerPos);
			//0.3921568627450980392156862745098 color
			ImGui::TextColored(ImVec4(0.341f, 0.51f, 0.17f, 1.0f), "Choose card to pay.");
			ImGui::GetFont()->Scale = old_size;
			ImGui::PopFont();

			const char* selectQuery = "SELECT cardnumber, ExpiryDate FROM Payment WHERE client_login = ?";
			sqlite3_stmt* selectStatement;
			rc = sqlite3_prepare_v2(db, selectQuery, -1, &selectStatement, 0);
			rc = sqlite3_bind_text(selectStatement, 1, User, -1, SQLITE_STATIC);

			std::vector<std::pair<std::string, std::string>> existingCards;

			while (sqlite3_step(selectStatement) == SQLITE_ROW) {
				const char* cardNumber = (const char*)sqlite3_column_text(selectStatement, 0);
				const char* expiryDate = (const char*)sqlite3_column_text(selectStatement, 1);

				if (cardNumber && expiryDate) {
					existingCards.emplace_back(std::make_pair(std::string(cardNumber), std::string(expiryDate)));
				}
			}
			sqlite3_finalize(selectStatement);

			if (!existingCards.empty()) {
				int countcards = 0;
				for (const auto& cardInfo : existingCards) {
					countcards++;
					std::string Childname = cardInfo.first;
					ImGui::BeginGroup();
					ImGui::BeginChild(Childname.c_str(), ImVec2(350.0f, 200.0f));
					if (cardInfo.first[0] == '2' || cardInfo.first[0] == '5') {
						centerPos = (ImGui::GetWindowSize().y + 50.f) * 0.50f;
						ImGui::SetCursorPosY(centerPos);
						centerPos = (ImGui::GetWindowSize().x + 180.f) * 0.50f;
						ImGui::SetCursorPosX(centerPos);
						auto image = Walnut::Application::Get().GetMastercard();
						ImGui::Image(image->GetDescriptorSet(), { 74, 74 });
					}
					else if (cardInfo.first[0] == '4') {
						centerPos = (ImGui::GetWindowSize().y + 50.f) * 0.50f;
						ImGui::SetCursorPosY(centerPos);
						centerPos = (ImGui::GetWindowSize().x + 180.f) * 0.50f;
						ImGui::SetCursorPosX(centerPos);
						auto image = Walnut::Application::Get().GetVisa_card();
						ImGui::Image(image->GetDescriptorSet(), { 74, 74 });
					}
					else if (cardInfo.first[0] == '3') {
						centerPos = (ImGui::GetWindowSize().y + 50.f) * 0.50f;
						ImGui::SetCursorPosY(centerPos);
						centerPos = (ImGui::GetWindowSize().x + 180.f) * 0.50f;
						ImGui::SetCursorPosX(centerPos);
						auto image = Walnut::Application::Get().GetAmericanExpress();
						ImGui::Image(image->GetDescriptorSet(), { 74, 74 });
					}
					ImGui::PushItemWidth(235.0f);
					centerPos = (ImGui::GetWindowSize().y - 0.f) * 0.50f;
					ImGui::SetCursorPosY(centerPos);
					centerPos = (ImGui::GetWindowSize().x - 270.f) * 0.50f;
					ImGui::SetCursorPosX(centerPos);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.6f));
					old_size = ImGui::GetFont()->Scale;
					ImGui::GetFont()->Scale += 0.25f;
					ImGui::PushFont(ImGui::GetFont());

					ImGui::Text(cardInfo.first.c_str());

					ImGui::GetFont()->Scale = old_size;
					ImGui::PopFont();
					ImGui::PopStyleColor();
					ImGui::PopStyleVar();
					centerPos = (ImGui::GetWindowSize().y + 100.f) * 0.50f;
					ImGui::SetCursorPosY(centerPos);
					centerPos = (ImGui::GetWindowSize().x - 150.f) * 0.50f;
					ImGui::SetCursorPosX(centerPos);
					ImGui::PushItemWidth(70.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.6f));

					ImGui::Text(cardInfo.second.c_str());

					ImGui::PopStyleColor();
					ImGui::PopStyleVar();
					ImGui::EndChild();
					ImGui::EndGroup();
					ImGui::SameLine();
					if (deletecard) {
						ImGui::BeginChild(Childname.c_str(), ImVec2(350.0f, 200.0f));

						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						centerPos = (ImGui::GetWindowSize().y - 200.f);
						ImGui::SetCursorPosY(centerPos);
						if (ImGui::Button(Childname.c_str(), ImVec2(350.0f, 200.0f))) {
							const char* deleteQuery = "DELETE FROM Payment WHERE client_login = ? AND cardnumber = ?";
							sqlite3_stmt* deleteStatement;
							const char* name = User;
							const char* cardNumber = cardInfo.first.c_str();
							rc = sqlite3_prepare_v2(db, deleteQuery, -1, &deleteStatement, 0);
							rc = sqlite3_bind_text(deleteStatement, 1, name, -1, SQLITE_STATIC);
							rc = sqlite3_bind_text(deleteStatement, 2, cardNumber, -1, SQLITE_STATIC);

							rc = sqlite3_step(deleteStatement);

							if (rc == SQLITE_DONE) {
								rc = sqlite3_finalize(deleteStatement);
								rc = sqlite3_exec(db, "COMMIT", 0, 0, 0);
								errortext = "Card deleted";
							}
							else {
								rc = sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
								errortext = "Error deleting card";
							}
						}
						ImGui::PopStyleColor(3);
						ImGui::PopStyleVar();

						ImGui::EndChild();
						if (countcards % 2 == 0) {
							ImGui::Spacing();
						}
						else {
							ImGui::SameLine();
						}
					}
					else {
						ImGui::BeginChild(Childname.c_str(), ImVec2(350.0f, 200.0f));

						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						centerPos = (ImGui::GetWindowSize().y - 200.f);
						ImGui::SetCursorPosY(centerPos);
						if (ImGui::Button(Childname.c_str(), ImVec2(350.0f, 200.0f))) {
							order_open = true;
							ImGui::SetWindowFocus("Order");
						}
						ImGui::PopStyleColor(3);
						ImGui::PopStyleVar();

						ImGui::EndChild();
						if (countcards % 2 == 0) {
							ImGui::Spacing();
						}
						else {
							ImGui::SameLine();
						}
					}
				}
				existingCards.clear();
			}

			static char cardnumber[20] = "";
			static char ExpiryDate[6] = "";
			static char CVC[4] = "";

			bool realnumber = false;
			bool realdate = false;
			if (card) {
				ImGui::Spacing();
				ImGui::BeginGroup();
				ImGui::BeginChild("new card", ImVec2(350.0f, 200.0f));
				if (cardnumber[0] == '2' || cardnumber[0] == '5') {
					centerPos = (ImGui::GetWindowSize().y + 50.f) * 0.50f;
					ImGui::SetCursorPosY(centerPos);
					centerPos = (ImGui::GetWindowSize().x + 180.f) * 0.50f;
					ImGui::SetCursorPosX(centerPos);
					auto image = Walnut::Application::Get().GetMastercard();
					ImGui::Image(image->GetDescriptorSet(), { 74, 74 });
					errortext = "";
					realnumber = true;
				}
				else if (cardnumber[0] == '4') {
					centerPos = (ImGui::GetWindowSize().y + 50.f) * 0.50f;
					ImGui::SetCursorPosY(centerPos);
					centerPos = (ImGui::GetWindowSize().x + 180.f) * 0.50f;
					ImGui::SetCursorPosX(centerPos);
					auto image = Walnut::Application::Get().GetVisa_card();
					ImGui::Image(image->GetDescriptorSet(), { 74, 74 });
					errortext = "";
					realnumber = true;
				}
				else if (cardnumber[0] == '3') {
					centerPos = (ImGui::GetWindowSize().y + 50.f) * 0.50f;
					ImGui::SetCursorPosY(centerPos);
					centerPos = (ImGui::GetWindowSize().x + 180.f) * 0.50f;
					ImGui::SetCursorPosX(centerPos);
					auto image = Walnut::Application::Get().GetAmericanExpress();
					ImGui::Image(image->GetDescriptorSet(), { 74, 74 });
					errortext = "";
					realnumber = true;
				}
				else {
					if (strlen(cardnumber) == 1) {
						errortext = "Invalid card number, your number need to start from:\n"
							"2 or 5 - Mastercard, 4 - Visa, 3 - American Express";
						realnumber = false;
					}
				}
				ImGui::PushItemWidth(235.0f);
				centerPos = (ImGui::GetWindowSize().y - 0.f) * 0.50f;
				ImGui::SetCursorPosY(centerPos);
				centerPos = (ImGui::GetWindowSize().x - 270.f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.6f));
				old_size = ImGui::GetFont()->Scale;
				ImGui::GetFont()->Scale += 0.25f;
				ImGui::PushFont(ImGui::GetFont());
				ImGui::InputTextWithHint("##number_input", "1234 5678 9012 3456", cardnumber, 20, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AlwaysOverwrite);
				ImGui::GetFont()->Scale = old_size;
				ImGui::PopFont();
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
				ImGui::SameLine();
				centerPos = (ImGui::GetWindowSize().y + 100.f) * 0.50f;
				ImGui::SetCursorPosY(centerPos);
				centerPos = (ImGui::GetWindowSize().x - 150.f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);
				ImGui::PushItemWidth(70.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.6f));
				ImGui::InputTextWithHint("##data", "09/25", ExpiryDate, 6, ImGuiInputTextFlags_EnterReturnsTrue);
				if (!IsValidCardNumber(cardnumber)) {
					errortext = "Invalid card number. Please enter a valid card number.";
					realnumber = false;
				}
				else if (!IsValidExpiryDate(ExpiryDate)) {
					errortext = "Invalid expiry date. Please enter a valid expiry date (MM/YY).";
					realdate = false;
				}
				else {
					errortext = "";
					realnumber = true;
					realdate = true;
				}
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("item views", ImVec2(350.0f, 200.0f));
				centerPos = (ImGui::GetWindowSize().y - 165.f) * 0.50f;
				ImGui::SetCursorPosY(centerPos);
				centerPos = (ImGui::GetWindowSize().x - 1000.f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);

				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				static ImVec4 colf = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
				const ImU32 col = ImColor(colf);
				const ImVec2 p = ImGui::GetCursorScreenPos();
				float x = p.x + 4.0f;
				float y = p.y + 4.0f;
				draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + 1000.f, y + 30.f), col);
				centerPos = (ImGui::GetWindowSize().y - 25.f) * 0.50f;
				ImGui::SetCursorPosY(centerPos);
				centerPos = (ImGui::GetWindowSize().x + 100.f) * 0.50f;
				ImGui::SetCursorPosX(centerPos);
				ImGui::PushItemWidth(60.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.6f));
				ImGui::InputTextWithHint("##cvc", " CVC ", CVC, 4, ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
				ImGui::EndChild();
				ImGui::EndGroup();
			}
			ImGui::Spacing();
			if (addactive) {
				if (ImGui::Button("Add card")) {
					addactive = true;
					card = true;
					deleteactive = false;
					deletecard = false;
					if (card && strlen(cardnumber) == 19 && strlen(ExpiryDate) == 5 && strlen(CVC) == 3 && realnumber && realdate) {
						const char* insertQuery = "INSERT INTO Payment VALUES (?, ?, ?, ?)";
						sqlite3_stmt* insertStatement;
						const char* name = User;
						rc = sqlite3_prepare_v2(db, insertQuery, -1, &insertStatement, 0);
						rc = sqlite3_bind_text(insertStatement, 1, name, -1, SQLITE_STATIC);
						rc = sqlite3_bind_text(insertStatement, 2, cardnumber, -1, SQLITE_STATIC);
						rc = sqlite3_bind_text(insertStatement, 3, ExpiryDate, -1, SQLITE_STATIC);
						rc = sqlite3_bind_text(insertStatement, 4, CVC, -1, SQLITE_STATIC);
						rc = sqlite3_step(insertStatement);
						if (rc == SQLITE_DONE) {
							rc = sqlite3_finalize(insertStatement);
							rc = sqlite3_exec(db, "COMMIT", 0, 0, 0);
							errortext = "Card added";
							card = false;
							deleteactive = true;
						}
						else {
							rc = sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
							errortext = "Error";
						}
					}
				}
			}
			if (deleteactive) {
				if (ImGui::Button("Delete card")) {
					deletecard = true;
				}
			}
			if (deletecard) {
				if (ImGui::Button("Cancel")) {
					deletecard = false;

				}
			}
			if (card) {
				if (ImGui::Button("Cancel")) {
					addactive = true;
					card = false;
					deletecard = false;
					deleteactive = true;
				}
			}
			ImGui::Text(errortext);
			//sqlite3_close(db);
		}
		ImGui::End();
	}
	static void ShowReceipt(bool* p_open) {
		if (!ImGui::Begin("E-receipt", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::End();
			return;
		}
		float old_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale += 0.2f;
		ImGui::PushFont(ImGui::GetFont());
		float centerPos = (ImGui::GetWindowSize().x - 150.f) * 0.50f;
		ImGui::SetCursorPosX(centerPos);
		ImGui::TextColored(ImVec4(0.64f, 0.04f, 0.15f, 1.0f), "24 Food Delivery");
		ImGui::GetFont()->Scale = old_size;
		ImGui::PopFont();

		ImGui::GetFont()->Scale += 0.2f;
		ImGui::PushFont(ImGui::GetFont());
		centerPos = (ImGui::GetWindowSize().x - 350.f) * 0.50f;
		ImGui::SetCursorPosX(centerPos);
		ImGui::Text("Address: 201 Mission St, San Francisco\n");
		ImGui::GetFont()->Scale = old_size;
		ImGui::PopFont();

		ImGui::TextUnformatted(
			"* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n"
		);
		ImGui::GetFont()->Scale += 0.2f;
		ImGui::PushFont(ImGui::GetFont());
		centerPos = (ImGui::GetWindowSize().x - 100.f) * 0.50f;
		ImGui::SetCursorPosX(centerPos);
		ImGui::Text("E-RECEIPT");
		ImGui::GetFont()->Scale = old_size;
		ImGui::PopFont();
		ImGui::TextUnformatted(
			"* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n"
		);
		static std::vector<int> itemsToDelete;
		static std::vector<int> countsToDelete;
		int itemCount;
		for (size_t i = 0; i < orderedFoods.size(); i++) {
			if (orderedFoods[i] != "") {
				const std::string& orderedFood = orderedFoods[i];
				const float orderedcost = orderedCost[i];

				auto it = foodCounts.find(orderedFood);
				if (it != foodCounts.end()) {
					itemCount = it->second;
					auto typeIt = foodTypes.find(selected);
					if (typeIt != foodTypes.end()) {
						//const FoodType& foodType = typeIt->second;
						if (orderedFood != "") {
							ImGui::Columns(2, "columns");

							ImGui::Text("%s", orderedFood.c_str()); ImGui::NextColumn();
							ImGui::Text("%d", itemCount);
							ImGui::SameLine();
							ImGui::Text("x%g", orderedcost);
							ImGui::SameLine();
							double multiplication = itemCount * orderedcost;
							ImGui::Text("= %g$", multiplication);
							ImGui::SameLine();
							ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.0f));
							ImGui::BeginChild(orderedFood.c_str(), ImVec2(20.f, 20.f));
							ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
							centerPos = (ImGui::GetWindowSize().x - 20.f);
							ImGui::SetCursorPosX(centerPos);
							auto image = Walnut::Application::Get().GetIconClose();
							ImGui::Image(image->GetDescriptorSet(), { 20, 20 });
							centerPos = (ImGui::GetWindowSize().y - 20.f);
							ImGui::SetCursorPosY(centerPos);
							centerPos = (ImGui::GetWindowSize().x - 20.f);
							ImGui::SetCursorPosX(centerPos);
							if (ImGui::Button(orderedFood.c_str(), ImVec2(20.f, 20.f))) {
								itemsToDelete.push_back(i);
								countsToDelete.push_back(itemCount);
							}
							ImGui::PopStyleColor(3);
							ImGui::PopStyleVar();
							ImGui::EndChild();
							ImGui::PopStyleColor();
							ImGui::NextColumn();
							ImGui::Separator();
						}
					}
				}
			}
		}
		for (int i = static_cast<int>(itemsToDelete.size()) - 1; i >= 0; i--) {
			int indexToDelete = itemsToDelete[i];
			int countToDelete = countsToDelete[i];

			// Subtract the cost of the deleted item from the total sum
			sum -= orderedCost[indexToDelete] * countToDelete;

			// Update the foodCounts map by decrementing the count of the deleted item
			const std::string& deletedItem = orderedFoods[indexToDelete];
			foodCounts[deletedItem] -= countToDelete;

			// Erase the item and its cost from the vectors if the count reaches 0
			if (foodCounts[deletedItem] == 0) {
				foodCounts.erase(deletedItem);
				orderedFoods.erase(orderedFoods.begin() + indexToDelete);
				orderedCost.erase(orderedCost.begin() + indexToDelete);
			}
			else {
				// If there are still remaining items of the same type, just erase it from the receipt
				orderedFoods.erase(orderedFoods.begin() + indexToDelete);
				orderedCost.erase(orderedCost.begin() + indexToDelete);
			}
		}
		// Clear the list of items to delete
		itemsToDelete.clear();
		countsToDelete.clear();
		ImGui::Columns(1);
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::GetFont()->Scale += 0.2f;
		ImGui::PushFont(ImGui::GetFont());
		centerPos = (ImGui::GetWindowSize().x - 100.f) * 0.50f;
		ImGui::SetCursorPosX(centerPos);
		//0.3921568627450980392156862745098 color
		ImGui::TextColored(ImVec4(0.341f, 0.51f, 0.17f, 1.0f), "Total: %g$", sum);
		ImGui::GetFont()->Scale = old_size;
		ImGui::PopFont();
		if (sum > 0) {
			if (ImGui::Button("Order", ImVec2(100.f, 40.f))) {
				payment_open = true;
				ImGui::SetWindowFocus("Payment");
			}

		}
		else {

			order_open = false;
			payment_open = false;
			receipt_open = false;



		}



		ImGui::End();
	}
	virtual void OnUIRender() override
	{
		//ImGui::ShowDemoWindow();
		if (load_open)
			ShowLoadLayout(&load_open);
		if (menu_open)
			ShowAppLayout(&menu_open);
		if (admin_open) {
			ShowAdminLayout(&admin_open);
		}
		if (receipt_open) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(500.0f, 100.0f), ImVec2(800.0f, 800.0f));
			ShowReceipt(&receipt_open);
		}
		if (payment_open)
			ShowPayment(&payment_open);
		if (order_open)
			ShowOrder(&order_open);
		if (history_open)
			ShowHistory(&history_open);

		UI_DrawAboutModal();
		UI_DrawOrderModal();
	}

	void UI_DrawAboutModal()
	{
		if (!m_AboutModalOpen)
			return;
		ImGui::OpenPopup("About");
		m_AboutModalOpen = ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (m_AboutModalOpen)
		{

			auto image = Walnut::Application::Get().GetApplicationIcon();
			ImGui::Image(image->GetDescriptorSet(), { 48, 48 });

			ImGui::SameLine();
			Walnut::UI::ShiftCursorX(20.0f);

			ImGui::BeginGroup();
			ImGui::Text("Application created");
			ImGui::Text("by Alex Chazov.");
			ImGui::EndGroup();

			if (Walnut::UI::ButtonCentered("Close"))
			{
				m_AboutModalOpen = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	void UI_DrawOrderModal()
	{
		if (!m_DrawOrderModal)
			return;
		ImGui::OpenPopup("Confirmation");
		m_DrawOrderModal = ImGui::BeginPopupModal("Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (m_DrawOrderModal)
		{

			auto image = Walnut::Application::Get().GetApplicationIcon();
			ImGui::Image(image->GetDescriptorSet(), { 48, 48 });

			ImGui::SameLine();
			Walnut::UI::ShiftCursorX(20.0f);

			ImGui::BeginGroup();
			ImGui::Text("Are you sure about your order?");
			ImGui::EndGroup();
			if (Walnut::UI::ButtonRight("Confirm"))
			{
				orderedFoods.clear();
				orderedCost.clear();
				sum = 0;

				order_open = false;
				payment_open = false;
				receipt_open = false;
				menu_open = false;
				history_open = true;

				m_DrawOrderModal = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (Walnut::UI::ButtonCentered("Not yet"))
			{
				m_DrawOrderModal = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	static void ShowDrawModal()
	{
		m_DrawOrderModal = true;
	}
	void ShowAboutModal()
	{
		m_AboutModalOpen = true;
	}
private:
	bool m_AboutModalOpen = false;

};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "24 Food Delivery";
	spec.CustomTitlebar = true;

	Walnut::Application* app = new Walnut::Application(spec);
	std::shared_ptr<NewLayer> Layer = std::make_shared<NewLayer>();
	app->PushLayer(Layer);
	app->SetMenubarCallback([app, Layer]()
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					app->Close();
				}
				ImGui::EndMenu();
			}
			if (!load_open && !admin_open) {
				if (ImGui::BeginMenu("Navigation"))
				{
					if (ImGui::MenuItem("Home"))
					{
						orderedFoods.clear();
						orderedCost.clear();
						foodCounts.clear();
						number = 0;
						sum = 0;

						order_open = false;
						payment_open = false;
						receipt_open = false;
						menu_open = true;
						history_open = false;
					}
					if (ImGui::MenuItem("History"))
					{
						orderedFoods.clear();
						orderedCost.clear();
						foodCounts.clear();
						number = 0;
						sum = 0;

						order_open = false;
						payment_open = false;
						receipt_open = false;
						menu_open = false;
						history_open = true;

					}
					if (ImGui::MenuItem("Sign out"))
					{
						orderedFoods.clear();
						orderedCost.clear();
						foodCounts.clear();
						number = 0;
						sum = 0;
						load_open = true;
						order_open = false;
						payment_open = false;
						receipt_open = false;
						menu_open = false;
						history_open = false;
					}
					ImGui::EndMenu();
				}
			}
			else if (admin_open) {
				if (ImGui::BeginMenu("Navigation"))
				{
					if (ImGui::MenuItem("Sign out"))
					{
						orderedFoods.clear();
						orderedCost.clear();
						foodCounts.clear();
						number = 0;
						sum = 0;
						load_open = true;
						admin_open = false;
						order_open = false;
						payment_open = false;
						receipt_open = false;
						menu_open = false;
						history_open = false;
					}
					ImGui::EndMenu();
				}
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("About"))
				{
					Layer->ShowAboutModal();
				}
				ImGui::EndMenu();
			}
		});
	return app;
}