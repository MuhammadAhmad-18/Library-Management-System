#include <SFML/Graphics.hpp>
#include <iostream> //used for input and output
#include <vector>
#include <conio.h>  //used for getch
#include <algorithm>   //for extra fucntions
#include <windows.h>
#include <mysql.h>  //the main connecting header
#include <sstream>
#include <iomanip>  //for using set w , left and right
#include <stdlib.h>
#include <cstdlib>
#include <fstream>


class Database {
    MYSQL* conn;
public:
    //thizs is basically constructor for automatically assigning the required values to connect the database
    Database() {
        conn = mysql_init(0);
        conn = mysql_real_connect(conn, "localhost", "root", "", "mydb", 3306, NULL, 0);
    }

    //this is Destructor , this will close the connection at the end
    ~Database() {
        if (conn) mysql_close(conn);
    }
    //the getter for the private member conn
    MYSQL* getConnection() { return conn; }
};

//basic functions
void viewBooksGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Fetch books from the database
    std::vector<std::vector<std::string>> books;
    std::string query = "SELECT * FROM books WHERE copies > 0;";
    int qstate = mysql_query(conn, query.c_str());
    if (qstate == 0) {
        MYSQL_RES* res = mysql_store_result(conn);
        if (res == nullptr) {
            std::cerr << "No books found or error occurred: " << mysql_error(conn) << std::endl;
            return;
        }

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            std::vector<std::string> book;
            for (int i = 0; i < mysql_num_fields(res); ++i) {
                book.push_back(row[i] ? row[i] : "NULL");
            }
            books.push_back(book);
        }

        // Free the result set
        mysql_free_result(res);
    } else {
        std::cerr << "Failed to retrieve books: " << mysql_error(conn) << std::endl;
        return;
    }

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);
    };

    updateLayout();

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the viewBooksGUI function and go back to the previous menu
                }

                // Existing code for other buttons
            }

            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        // Display table headers
        sf::Text header;
        header.setFont(font);
        header.setCharacterSize(24);
        header.setFillColor(sf::Color::White);
        header.setStyle(sf::Text::Bold);

        std::vector<std::string> headers = { "Book ID", "Title", "Author", "Copies" };
        float xPos = 50.0f;
        float yPos = 50.0f;

        for (const auto& h : headers) {
            header.setString(h);
            header.setPosition(xPos, yPos);
            window.draw(header);
            xPos += 200.0f; // Adjust spacing between columns
        }

        yPos += 40.0f; // Move to the next row

        // Display book data
        for (const auto& book : books) {
            sf::Text text;
            text.setFont(font);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::White);

            xPos = 50.0f;
            for (const auto& field : book) {
                text.setString(field);
                text.setPosition(xPos, yPos);
                window.draw(text);
                xPos += 200.0f; // Adjust spacing between columns
            }

            yPos += 30.0f; // Move to the next row
        }

        window.display();
    }
}

void searchBookGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input field setup
    sf::RectangleShape inputBox(sf::Vector2f(400, 50));
    inputBox.setFillColor(sf::Color::White);
    inputBox.setPosition(50, 50);

    sf::Text inputText;
    inputText.setFont(font);
    inputText.setCharacterSize(24);
    inputText.setFillColor(sf::Color::Black);
    inputText.setPosition(60, 60);

    std::string userInput;

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 60);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Search button setup
    sf::RectangleShape searchButton(sf::Vector2f(100, 50));
    searchButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    searchButton.setOutlineColor(sf::Color::White);
    searchButton.setOutlineThickness(1);
    searchButton.setPosition(470, 50);

    sf::Text searchButtonText("Search", font, 24);
    searchButtonText.setFillColor(sf::Color::White);
    searchButtonText.setPosition(485, 60);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);
    };

    updateLayout();

    // Vector to store search results
    std::vector<std::vector<std::string>> searchResults;

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b' && !userInput.empty()) { // Handle backspace
                    userInput.pop_back();
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    userInput += static_cast<char>(event.text.unicode);
                }
                inputText.setString(userInput);
                cursor.setPosition(60 + inputText.getLocalBounds().width + 5, 60);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (searchButton.getGlobalBounds().contains(mousePos)) {
                    // Execute search query
                    searchResults.clear();
                    std::string query = "SELECT * FROM books WHERE title LIKE '%" + userInput + "%';";
                    int qstate = mysql_query(conn, query.c_str());
                    if (qstate == 0) {
                        MYSQL_RES* res = mysql_store_result(conn);
                        if (res == nullptr) {
                            std::cerr << "Failed to retrieve results: " << mysql_error(conn) << std::endl;
                        } else {
                            MYSQL_ROW row;
                            while ((row = mysql_fetch_row(res))) {
                                std::vector<std::string> book;
                                for (int i = 0; i < mysql_num_fields(res); ++i) {
                                    book.push_back(row[i] ? row[i] : "NULL");
                                }
                                searchResults.push_back(book);
                            }
                            mysql_free_result(res);
                        }
                    } else {
                        std::cerr << "Failed to search books: " << mysql_error(conn) << std::endl;
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the searchBookGUI function and go back to the previous menu
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input field and search button
        window.draw(inputBox);
        window.draw(inputText);
        window.draw(searchButton);
        window.draw(searchButtonText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        // Display search results
        float yPos = 150.0f; // Start y position for table
        if (!searchResults.empty()) {
            // Display table headers
            sf::Text header;
            header.setFont(font);
            header.setCharacterSize(24);
            header.setFillColor(sf::Color::White);
            header.setStyle(sf::Text::Bold);

            std::vector<std::string> headers = { "Book ID", "Title", "Author", "Copies" };
            float xPos = 50.0f;

            for (const auto& h : headers) {
                header.setString(h);
                header.setPosition(xPos, yPos);
                window.draw(header);
                xPos += 200.0f; // Adjust spacing between columns
            }

            yPos += 40.0f; // Move to the next row

            // Display book data
            for (const auto& book : searchResults) {
                sf::Text text;
                text.setFont(font);
                text.setCharacterSize(20);
                text.setFillColor(sf::Color::White);

                xPos = 50.0f;
                for (const auto& field : book) {
                    text.setString(field);
                    text.setPosition(xPos, yPos);
                    window.draw(text);
                    xPos += 200.0f; // Adjust spacing between columns
                }

                yPos += 30.0f; // Move to the next row
            }
        }

        window.display();
    }
}

void returnBookGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input fields setup
    sf::RectangleShape studentIdBox(sf::Vector2f(400, 50));
    studentIdBox.setFillColor(sf::Color::White);
    studentIdBox.setPosition(50, 100);

    sf::Text studentIdText;
    studentIdText.setFont(font);
    studentIdText.setCharacterSize(24);
    studentIdText.setFillColor(sf::Color::Black);
    studentIdText.setPosition(60, 110);

    std::string studentIdInput;

    sf::RectangleShape bookIdBox(sf::Vector2f(400, 50));
    bookIdBox.setFillColor(sf::Color::White);
    bookIdBox.setPosition(50, 200);

    sf::Text bookIdText;
    bookIdText.setFont(font);
    bookIdText.setCharacterSize(24);
    bookIdText.setFillColor(sf::Color::Black);
    bookIdText.setPosition(60, 210);

    std::string bookIdInput;

    // Labels for input fields
    sf::Text studentIdLabel("Student ID:", font, 24);
    studentIdLabel.setFillColor(sf::Color::White);
    studentIdLabel.setPosition(50, 70);

    sf::Text bookIdLabel("Book ID:", font, 24);
    bookIdLabel.setFillColor(sf::Color::White);
    bookIdLabel.setPosition(50, 170);

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Return button setup
    sf::RectangleShape returnButton(sf::Vector2f(100, 50));
    returnButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    returnButton.setOutlineColor(sf::Color::White);
    returnButton.setOutlineThickness(1);
    returnButton.setPosition(window.getSize().x - 150, window.getSize().y - 100);

    sf::Text returnButtonText("Return", font, 24);
    returnButtonText.setFillColor(sf::Color::White);
    returnButtonText.setPosition(window.getSize().x - 135, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 300);

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 310);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the return button and its text
        returnButton.setPosition(windowSize.x - 150, windowSize.y - 100);
        returnButtonText.setPosition(windowSize.x - 135, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 300);
        messageText.setPosition(60, 310);
    };

    updateLayout();

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!studentIdInput.empty()) {
                            studentIdInput.pop_back();
                        }
                    } else if (bookIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!bookIdInput.empty()) {
                            bookIdInput.pop_back();
                        }
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    char enteredChar = static_cast<char>(event.text.unicode);
                    if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        studentIdInput += enteredChar;
                    } else if (bookIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        bookIdInput += enteredChar;
                    }
                }
                studentIdText.setString(studentIdInput);
                bookIdText.setString(bookIdInput);
                cursor.setPosition(60 + (studentIdBox.getGlobalBounds().contains(cursor.getPosition()) ? studentIdText.getLocalBounds().width : bookIdText.getLocalBounds().width) + 5, cursor.getPosition().y);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (returnButton.getGlobalBounds().contains(mousePos)) {
                    // Execute return book query
                    int studentId = std::stoi(studentIdInput);
                    int bookId = std::stoi(bookIdInput);

                    // Check if the student has issued the book
                    std::string checkQuery = "SELECT issue_id FROM issued_books WHERE student_id = " + std::to_string(studentId) + " AND book_id = " + std::to_string(bookId) + " AND return_date IS NULL;";
                    if (mysql_query(conn, checkQuery.c_str()) == 0) {
                        MYSQL_RES* res = mysql_store_result(conn);
                        if (mysql_num_rows(res) == 0) {
                            messageText.setString("No record found for this book being issued to the student.");
                            mysql_free_result(res);
                            continue;
                        }

                        MYSQL_ROW row = mysql_fetch_row(res);
                        int issueId = atoi(row[0]);
                        mysql_free_result(res);

                        // Update return_date
                        std::string returnQuery = "UPDATE issued_books SET return_date = CURRENT_DATE WHERE issue_id = " + std::to_string(issueId) + ";";
                        if (mysql_query(conn, returnQuery.c_str()) == 0) {
                            // Update the book's available copies
                            std::string updateQuery = "UPDATE books SET copies = copies + 1 WHERE book_id = " + std::to_string(bookId) + ";";
                            if (mysql_query(conn, updateQuery.c_str()) == 0) {
                                messageText.setFillColor(sf::Color::Green);
                                messageText.setString("Book returned successfully!");
                            } else {
                                messageText.setString("Failed to update book copies: " + std::string(mysql_error(conn)));
                            }
                        } else {
                            messageText.setString("Failed to update return date: " + std::string(mysql_error(conn)));
                        }
                    } else {
                        messageText.setString("Failed to check issued book record: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the returnBookGUI function and go back to the previous menu
                }

                // Move cursor to the clicked input box
                if (studentIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + studentIdText.getLocalBounds().width + 5, 110);
                } else if (bookIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + bookIdText.getLocalBounds().width + 5, 210);
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input fields, labels, and message box
        window.draw(studentIdLabel);
        window.draw(studentIdBox);
        window.draw(studentIdText);
        window.draw(bookIdLabel);
        window.draw(bookIdBox);
        window.draw(bookIdText);
        window.draw(messageBox);
        window.draw(messageText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the return button
        window.draw(returnButton);
        window.draw(returnButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}


void issueBookGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input fields setup
    sf::RectangleShape studentIdBox(sf::Vector2f(400, 50));
    studentIdBox.setFillColor(sf::Color::White);
    studentIdBox.setPosition(50, 100);

    sf::Text studentIdText;
    studentIdText.setFont(font);
    studentIdText.setCharacterSize(24);
    studentIdText.setFillColor(sf::Color::Black);
    studentIdText.setPosition(60, 110);

    std::string studentIdInput;

    sf::RectangleShape bookIdBox(sf::Vector2f(400, 50));
    bookIdBox.setFillColor(sf::Color::White);
    bookIdBox.setPosition(50, 200);

    sf::Text bookIdText;
    bookIdText.setFont(font);
    bookIdText.setCharacterSize(24);
    bookIdText.setFillColor(sf::Color::Black);
    bookIdText.setPosition(60, 210);

    std::string bookIdInput;

    // Labels for input fields
    sf::Text studentIdLabel("Student ID:", font, 24);
    studentIdLabel.setFillColor(sf::Color::White);
    studentIdLabel.setPosition(50, 70);

    sf::Text bookIdLabel("Book ID:", font, 24);
    bookIdLabel.setFillColor(sf::Color::White);
    bookIdLabel.setPosition(50, 170);

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Issue button setup
    sf::RectangleShape issueButton(sf::Vector2f(100, 50));
    issueButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    issueButton.setOutlineColor(sf::Color::White);
    issueButton.setOutlineThickness(1);
    issueButton.setPosition(window.getSize().x - 150, window.getSize().y - 100);

    sf::Text issueButtonText("Issue", font, 24);
    issueButtonText.setFillColor(sf::Color::White);
    issueButtonText.setPosition(window.getSize().x - 135, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 300);

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 310);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the issue button and its text
        issueButton.setPosition(windowSize.x - 150, windowSize.y - 100);
        issueButtonText.setPosition(windowSize.x - 135, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 300);
        messageText.setPosition(60, 310);
    };

    updateLayout();

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!studentIdInput.empty()) {
                            studentIdInput.pop_back();
                        }
                    } else if (bookIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!bookIdInput.empty()) {
                            bookIdInput.pop_back();
                        }
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    char enteredChar = static_cast<char>(event.text.unicode);
                    if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        studentIdInput += enteredChar;
                    } else if (bookIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        bookIdInput += enteredChar;
                    }
                }
                studentIdText.setString(studentIdInput);
                bookIdText.setString(bookIdInput);
                cursor.setPosition(60 + (studentIdBox.getGlobalBounds().contains(cursor.getPosition()) ? studentIdText.getLocalBounds().width : bookIdText.getLocalBounds().width) + 5, cursor.getPosition().y);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (issueButton.getGlobalBounds().contains(mousePos)) {
                    // Execute issue book query
                    int studentId = std::stoi(studentIdInput);
                    int bookId = std::stoi(bookIdInput);

                    // Check if student is eligible for issuing the book
                    std::string checkStudent = "SELECT * FROM students WHERE student_id = " + std::to_string(studentId) + ";";
                    if (mysql_query(conn, checkStudent.c_str()) == 0) {
                        MYSQL_RES* res = mysql_store_result(conn);
                        if (res == nullptr) {
                            messageText.setString("Failed to receive the data");
                            continue;
                        }

                        MYSQL_ROW row;
                        int noOfrows = mysql_num_rows(res);
                        if (noOfrows == 0) {
                            messageText.setString("Student not registered. Please contact the librarian for registration");
                            mysql_free_result(res);
                            continue;
                        }
                        mysql_free_result(res);
                    } else {
                        messageText.setString("Failed to find the student: " + std::string(mysql_error(conn)));
                        continue;
                    }

                    // Check if the book is available
                    std::string checkQuery = "SELECT copies FROM books WHERE book_id = " + std::to_string(bookId) + " AND copies > 0;";
                    int qstate1 = mysql_query(conn, checkQuery.c_str());
                    if (qstate1 == 0) {
                        MYSQL_RES* res = mysql_store_result(conn);
                        if (mysql_num_rows(res) == 0) {
                            messageText.setString("Book is not available for issue.");
                            mysql_free_result(res);
                            continue;
                        }
                        mysql_free_result(res);
                    } else {
                        messageText.setString("Failed to check book availability: " + std::string(mysql_error(conn)));
                        continue;
                    }

                    // Issue the book
                    std::string issueQuery = "INSERT INTO issued_books (student_id, book_id) VALUES (" + std::to_string(studentId) + ", " + std::to_string(bookId) + ");";
                    int qstate2 = mysql_query(conn, issueQuery.c_str());
                    if (qstate2 == 0) {
                        // Update the book's available copies
                        std::string updateQuery = "UPDATE books SET copies = copies - 1 WHERE book_id = " + std::to_string(bookId) + ";";
                        if (mysql_query(conn, updateQuery.c_str()) == 0) {
                            messageText.setFillColor(sf::Color::Green);
                            messageText.setString("Book issued successfully!");
                        } else {
                            messageText.setString("Failed to update book copies: " + std::string(mysql_error(conn)));
                        }
                    } else {
                        messageText.setString("Failed to issue book: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the issueBookGUI function and go back to the previous menu
                }

                // Move cursor to the clicked input box
                if (studentIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + studentIdText.getLocalBounds().width + 5, 110);
                } else if (bookIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + bookIdText.getLocalBounds().width + 5, 210);
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input fields, labels, and message box
        window.draw(studentIdLabel);
        window.draw(studentIdBox);
        window.draw(studentIdText);
        window.draw(bookIdLabel);
        window.draw(bookIdBox);
        window.draw(bookIdText);
        window.draw(messageBox);
        window.draw(messageText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the issue button
        window.draw(issueButton);
        window.draw(issueButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}


void studentMenu(sf::RenderWindow& window, sf::Font& font, sf::Sprite& background,MYSQL* conn) {
    // Student Menu Setup
    std::vector<sf::RectangleShape> studentMenuButtons;
    std::vector<sf::Text> studentMenuTexts;

     // Load custom background texture for book view
    sf::Texture solidTexture;
    if (!solidTexture.loadFromFile("solid.png")) {
        std::cerr << "Error: Could not load 'solid.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture searchTexture;
    if (!searchTexture.loadFromFile("search.png")) {
        std::cerr << "Error: Could not load 'search.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture issueTexture;
    if (!issueTexture.loadFromFile("issue.png")) {
        std::cerr << "Error: Could not load 'issueTexture.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture returnTexture;
    if (!returnTexture.loadFromFile("return.jpg")) {
        std::cerr << "Error: Could not load 'returnTexture.jpg'. Ensure the file exists in the correct directory.\n";
        return;
    }


    float buttonWidth = 200;
    float buttonHeight = 50;

    std::vector<std::string> studentMenuLabels = {
        "View Books", "Search Books", "Issue Books", "Return Book", "Back"
    };

    for (size_t i = 0; i < studentMenuLabels.size(); ++i) {
        // Create button shape
        sf::RectangleShape button(sf::Vector2f(buttonWidth, buttonHeight));
        button.setPosition((window.getSize().x - buttonWidth) / 2, 200 + i * (buttonHeight + 20));
        button.setFillColor(sf::Color(93, 64, 55)); // Default brown color

        // Set outline color and thickness
        button.setOutlineColor(sf::Color::White);
        button.setOutlineThickness(2);

        studentMenuButtons.push_back(button);

        // Create button text
        sf::Text text;
        text.setFont(font);
        text.setString(studentMenuLabels[i]);
        text.setCharacterSize(20);
        text.setFillColor(sf::Color::White);
        text.setPosition(
        button.getPosition().x + (buttonWidth - text.getLocalBounds().width) / 2,
        button.getPosition().y + (buttonHeight - text.getLocalBounds().height) / 2);
        studentMenuTexts.push_back(text);
    }

    // Loop to handle Student Menu events
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                for (size_t i = 0; i < studentMenuButtons.size(); ++i) {
                    if (studentMenuButtons[i].getGlobalBounds().contains(mousePos)) {
                        if (studentMenuTexts[i].getString() == "Back") {
                            return; // Exit student menu
                        } else if (studentMenuTexts[i].getString() == "View Books") {
                            viewBooksGUI(window, font, solidTexture, conn); // Call the viewBooksGUI function
                        } else if (studentMenuTexts[i].getString() == "Search Books") {
                            searchBookGUI(window, font, searchTexture, conn); // Call the searchBookGUI function
                        } else if (studentMenuTexts[i].getString() == "Issue Books") {
                            issueBookGUI(window, font, issueTexture, conn); // Call the issueBookGUI function
                        } else if (studentMenuTexts[i].getString() == "Return Book") {
                            returnBookGUI(window, font, returnTexture, conn); // Call the returnBookGUI function
                        } else {
                            std::cout << studentMenuTexts[i].getString().toAnsiString() << " clicked!\n";
                        }
                    }

                }
            }
        }

        // Rendering
        window.clear();

        // Draw the background
        window.draw(background);

        // Draw student menu buttons
        for (size_t i = 0; i < studentMenuButtons.size(); ++i) {
            window.draw(studentMenuButtons[i]);
            window.draw(studentMenuTexts[i]);
        }

        window.display();
    }
}

void addBookGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input fields setup
    sf::RectangleShape titleBox(sf::Vector2f(400, 50));
    titleBox.setFillColor(sf::Color::White);
    titleBox.setPosition(50, 100);

    sf::Text titleText;
    titleText.setFont(font);
    titleText.setCharacterSize(24);
    titleText.setFillColor(sf::Color::Black);
    titleText.setPosition(60, 110);

    std::string titleInput;

    sf::RectangleShape authorBox(sf::Vector2f(400, 50));
    authorBox.setFillColor(sf::Color::White);
    authorBox.setPosition(50, 200);

    sf::Text authorText;
    authorText.setFont(font);
    authorText.setCharacterSize(24);
    authorText.setFillColor(sf::Color::Black);
    authorText.setPosition(60, 210);

    std::string authorInput;

    sf::RectangleShape copiesBox(sf::Vector2f(400, 50));
    copiesBox.setFillColor(sf::Color::White);
    copiesBox.setPosition(50, 300);

    sf::Text copiesText;
    copiesText.setFont(font);
    copiesText.setCharacterSize(24);
    copiesText.setFillColor(sf::Color::Black);
    copiesText.setPosition(60, 310);

    std::string copiesInput;

    // Labels for input fields
    sf::Text titleLabel("Title:", font, 24);
    titleLabel.setFillColor(sf::Color::White);
    titleLabel.setPosition(50, 70);

    sf::Text authorLabel("Author:", font, 24);
    authorLabel.setFillColor(sf::Color::White);
    authorLabel.setPosition(50, 170);

    sf::Text copiesLabel("Copies:", font, 24);
    copiesLabel.setFillColor(sf::Color::White);
    copiesLabel.setPosition(50, 270);

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Add button setup
    sf::RectangleShape addButton(sf::Vector2f(100, 50));
    addButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    addButton.setOutlineColor(sf::Color::White);
    addButton.setOutlineThickness(1);
    addButton.setPosition(window.getSize().x - 150, window.getSize().y - 100);

    sf::Text addButtonText("Add", font, 24);
    addButtonText.setFillColor(sf::Color::White);
    addButtonText.setPosition(window.getSize().x - 125, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 400);

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 410);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the add button and its text
        addButton.setPosition(windowSize.x - 150, windowSize.y - 100);
        addButtonText.setPosition(windowSize.x - 125, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 400);
        messageText.setPosition(60, 410);
    };

    updateLayout();

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (titleBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!titleInput.empty()) {
                            titleInput.pop_back();
                        }
                    } else if (authorBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!authorInput.empty()) {
                            authorInput.pop_back();
                        }
                    } else if (copiesBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!copiesInput.empty()) {
                            copiesInput.pop_back();
                        }
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    char enteredChar = static_cast<char>(event.text.unicode);
                    if (titleBox.getGlobalBounds().contains(cursor.getPosition())) {
                        titleInput += enteredChar;
                    } else if (authorBox.getGlobalBounds().contains(cursor.getPosition())) {
                        authorInput += enteredChar;
                    } else if (copiesBox.getGlobalBounds().contains(cursor.getPosition())) {
                        copiesInput += enteredChar;
                    }
                }
                titleText.setString(titleInput);
                authorText.setString(authorInput);
                copiesText.setString(copiesInput);
                cursor.setPosition(60 + (titleBox.getGlobalBounds().contains(cursor.getPosition()) ? titleText.getLocalBounds().width : authorBox.getGlobalBounds().contains(cursor.getPosition()) ? authorText.getLocalBounds().width : copiesText.getLocalBounds().width) + 5, cursor.getPosition().y);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (addButton.getGlobalBounds().contains(mousePos)) {
                    // Execute add book query
                    int copies = std::stoi(copiesInput);

                    std::string query = "INSERT INTO books (title, author, copies) VALUES ('" + titleInput + "', '" + authorInput + "', " + std::to_string(copies) + ");";
                    if (mysql_query(conn, query.c_str()) == 0) {
                        messageText.setFillColor(sf::Color::Green);
                        messageText.setString("Book added successfully!");
                    } else {
                        messageText.setString("Failed to add book: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the addBookGUI function and go back to the previous menu
                }

                // Move cursor to the clicked input box
                if (titleBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + titleText.getLocalBounds().width + 5, 110);
                } else if (authorBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + authorText.getLocalBounds().width + 5, 210);
                } else if (copiesBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + copiesText.getLocalBounds().width + 5, 310);
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input fields, labels, and message box
        window.draw(titleLabel);
        window.draw(titleBox);
        window.draw(titleText);
        window.draw(authorLabel);
        window.draw(authorBox);
        window.draw(authorText);
        window.draw(copiesLabel);
        window.draw(copiesBox);
        window.draw(copiesText);
        window.draw(messageBox);
        window.draw(messageText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the add button
        window.draw(addButton);
        window.draw(addButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}

void deleteBookGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {

    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input field setup
    sf::RectangleShape bookIdBox(sf::Vector2f(400, 50));
    bookIdBox.setFillColor(sf::Color::White);
    bookIdBox.setPosition(50, 100);

    sf::Text bookIdText;
    bookIdText.setFont(font);
    bookIdText.setCharacterSize(24);
    bookIdText.setFillColor(sf::Color::Black);
    bookIdText.setPosition(60, 110);

    std::string bookIdInput;

    // Label for input field
    sf::Text bookIdLabel("Book ID:", font, 24);
    bookIdLabel.setFillColor(sf::Color::White);
    bookIdLabel.setPosition(50, 70);

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Delete button setup
    sf::RectangleShape deleteButton(sf::Vector2f(100, 50));
    deleteButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    deleteButton.setOutlineColor(sf::Color::White);
    deleteButton.setOutlineThickness(1);
    deleteButton.setPosition(window.getSize().x - 150, window.getSize().y - 100);

    sf::Text deleteButtonText("Delete", font, 24);
    deleteButtonText.setFillColor(sf::Color::White);
    deleteButtonText.setPosition(window.getSize().x - 135, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 200);

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 210);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the delete button and its text
        deleteButton.setPosition(windowSize.x - 150, windowSize.y - 100);
        deleteButtonText.setPosition(windowSize.x - 135, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 200);
        messageText.setPosition(60, 210);
    };

    updateLayout();

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (!bookIdInput.empty()) {
                        bookIdInput.pop_back();
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    bookIdInput += static_cast<char>(event.text.unicode);
                }
                bookIdText.setString(bookIdInput);
                cursor.setPosition(60 + bookIdText.getLocalBounds().width + 5, 110);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (deleteButton.getGlobalBounds().contains(mousePos)) {
                    // Execute delete book query
                    int bookId = std::stoi(bookIdInput);

                    std::string query = "DELETE FROM books WHERE book_id = " + std::to_string(bookId) + ";";
                    if (mysql_query(conn, query.c_str()) == 0) {
                        messageText.setFillColor(sf::Color::Green);
                        messageText.setString("Book deleted successfully!");
                    } else {
                        messageText.setString("Failed to delete book: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the deleteBookGUI function and go back to the previous menu
                }

                // Move cursor to the clicked input box
                if (bookIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + bookIdText.getLocalBounds().width + 5, 110);
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input field, label, and message box
        window.draw(bookIdLabel);
        window.draw(bookIdBox);
        window.draw(bookIdText);
        window.draw(messageBox);
        window.draw(messageText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the delete button
        window.draw(deleteButton);
        window.draw(deleteButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}

void modifyBookGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input field setup
    sf::RectangleShape bookIdBox(sf::Vector2f(400, 50));
    bookIdBox.setFillColor(sf::Color::White);
    bookIdBox.setPosition(50, 100);

    sf::Text bookIdText;
    bookIdText.setFont(font);
    bookIdText.setCharacterSize(24);
    bookIdText.setFillColor(sf::Color::Black);
    bookIdText.setPosition(60, 110);

    std::string bookIdInput;

    // Label for input field
    sf::Text bookIdLabel("Book ID:", font, 24);
    bookIdLabel.setFillColor(sf::Color::White);
    bookIdLabel.setPosition(50, 70);

    // Dropdown for selecting field to modify
    sf::RectangleShape dropdownBox(sf::Vector2f(400, 50));
    dropdownBox.setFillColor(sf::Color::White);
    dropdownBox.setPosition(50, 200);

    sf::Text dropdownText;
    dropdownText.setFont(font);
    dropdownText.setCharacterSize(24);
    dropdownText.setFillColor(sf::Color::Black);
    dropdownText.setPosition(60, 210);

    std::vector<std::string> dropdownOptions = {"Book ID", "Book Title", "Author", "Copies"};
    int selectedOption = -1;
    bool dropdownOpen = false; // To track if the dropdown is open

    // Input field for new value
    sf::RectangleShape newValueBox(sf::Vector2f(400, 50));
    newValueBox.setFillColor(sf::Color::White);
    newValueBox.setPosition(50, 400);  // Moved up slightly to avoid overlap with back button

    sf::Text newValueText;
    newValueText.setFont(font);
    newValueText.setCharacterSize(24);
    newValueText.setFillColor(sf::Color::Black);
    newValueText.setPosition(60, 410);

    std::string newValueInput;

    // Labels for the dropdown and new value input field
    sf::Text dropdownLabel("Field to Modify:", font, 24);
    dropdownLabel.setFillColor(sf::Color::White);
    dropdownLabel.setPosition(50, 170);

    sf::Text newValueLabel("New Value:", font, 24);
    newValueLabel.setFillColor(sf::Color::White);
    newValueLabel.setPosition(50, 370);  // Moved up slightly to avoid overlap with back button

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Save button setup
    sf::RectangleShape saveButton(sf::Vector2f(100, 50));
    saveButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    saveButton.setOutlineColor(sf::Color::White);
    saveButton.setOutlineThickness(1);
    saveButton.setPosition(window.getSize().x - 150, window.getSize().y - 100);

    sf::Text saveButtonText("Save", font, 24);
    saveButtonText.setFillColor(sf::Color::White);
    saveButtonText.setPosition(window.getSize().x - 125, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 500);  // Adjusted position

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 510);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the save button and its text
        saveButton.setPosition(windowSize.x - 150, windowSize.y - 100);
        saveButtonText.setPosition(windowSize.x - 125, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 500);
        messageText.setPosition(60, 510);
    };

    updateLayout();

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (bookIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!bookIdInput.empty()) {
                            bookIdInput.pop_back();
                        }
                    } else if (newValueBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!newValueInput.empty()) {
                            newValueInput.pop_back();
                        }
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    char enteredChar = static_cast<char>(event.text.unicode);
                    if (bookIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        bookIdInput += enteredChar;
                    } else if (newValueBox.getGlobalBounds().contains(cursor.getPosition())) {
                        newValueInput += enteredChar;
                    }
                }
                bookIdText.setString(bookIdInput);
                newValueText.setString(newValueInput);
                cursor.setPosition(60 + (bookIdBox.getGlobalBounds().contains(cursor.getPosition()) ? bookIdText.getLocalBounds().width : newValueText.getLocalBounds().width) + 5, cursor.getPosition().y);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (saveButton.getGlobalBounds().contains(mousePos)) {
                    // Execute modify book query
                    int bookId = std::stoi(bookIdInput);
                    std::string query;

                    if (selectedOption == 0) { // Modify Book ID
                        int newBookId = std::stoi(newValueInput);
                        query = "UPDATE books SET book_id = " + std::to_string(newBookId) + " WHERE book_id = " + std::to_string(bookId) + ";";
                    } else if (selectedOption == 1) { // Modify Book Title
                        query = "UPDATE books SET title = '" + newValueInput + "' WHERE book_id = " + std::to_string(bookId) + ";";
                    } else if (selectedOption == 2) { // Modify Author
                        query = "UPDATE books SET author = '" + newValueInput + "' WHERE book_id = " + std::to_string(bookId) + ";";
                    } else if (selectedOption == 3) { // Modify Copies
                        int newCopies = std::stoi(newValueInput);
                        query = "UPDATE books SET copies = " + std::to_string(newCopies) + " WHERE book_id = " + std::to_string(bookId) + ";";
                    } else {
                        messageText.setString("Please select a field to modify.");
                        continue;
                    }

                    if (mysql_query(conn, query.c_str()) == 0) {
                        messageText.setFillColor(sf::Color::Green);
                        messageText.setString("Book modified successfully!");
                    } else {
                        messageText.setString("Failed to modify book: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the modifyBookGUI function and go back to the previous menu
                }

                // Move cursor to the clicked input box
                if (bookIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + bookIdText.getLocalBounds().width + 5, 110);
                } else if (newValueBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + newValueText.getLocalBounds().width + 5, 410);
                } else if (dropdownBox.getGlobalBounds().contains(mousePos)) {
                    dropdownOpen = !dropdownOpen; // Toggle the dropdown open state
                } else if (dropdownOpen) {
                    // Check if an option was clicked
                    for (size_t i = 0; i < dropdownOptions.size(); ++i) {
                        sf::FloatRect optionBounds(60, 260 + i * 30, 400, 30); // Adjusted position to avoid overlap
                        if (optionBounds.contains(mousePos)) {
                            selectedOption = i;
                            dropdownText.setString(dropdownOptions[i]);
                            dropdownOpen = false; // Close the dropdown
                            break;
                        }
                    }
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input fields, labels, and message box
        window.draw(bookIdLabel);
        window.draw(bookIdBox);
        window.draw(bookIdText);
        window.draw(dropdownLabel);
        window.draw(dropdownBox);
        window.draw(dropdownText);
        window.draw(newValueLabel);
        window.draw(newValueBox);
        window.draw(newValueText);
        window.draw(messageBox);
        window.draw(messageText);

        // Draw dropdown options if open
        if (dropdownOpen) {
            for (size_t i = 0; i < dropdownOptions.size(); ++i) {
                sf::Text optionText(dropdownOptions[i], font, 24);
                optionText.setFillColor(sf::Color::White);
                optionText.setPosition(60, 260 + i * 30); // Adjusted position to avoid overlap
                window.draw(optionText);
            }
        }

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the save button
        window.draw(saveButton);
        window.draw(saveButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}


void viewIssuedBookGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Table headers setup
    sf::Text headers[5];
    std::string headerLabels[5] = {"Issue ID", "Student ID", "Book ID", "Issue Date", "Return Date"};
    for (int i = 0; i < 5; ++i) {
        headers[i].setFont(font);
        headers[i].setString(headerLabels[i]);
        headers[i].setCharacterSize(24);
        headers[i].setFillColor(sf::Color::White);
        headers[i].setPosition(50 + i * 150, 50);
    }

    // Query to fetch all issued books
    std::string viewIssued = "SELECT * FROM issued_books";
    MYSQL_RES* res;
    MYSQL_ROW row;
    std::vector<std::vector<std::string>> issuedBooks;

    // Execute the query
    if (mysql_query(conn, viewIssued.c_str()) == 0) {
        res = mysql_store_result(conn);

        if (res != nullptr) {
            // Fetch all rows and store them in issuedBooks
            while ((row = mysql_fetch_row(res))) {
                std::vector<std::string> bookRow;
                for (int i = 0; i < 5; ++i) {
                    bookRow.push_back(row[i] ? row[i] : "NULL");
                }
                issuedBooks.push_back(bookRow);
            }
            mysql_free_result(res);
        } else {
            std::cerr << "Failed to fetch result set: " << mysql_error(conn) << std::endl;
            return;
        }
    } else {
        std::cerr << "Failed to execute query: " << mysql_error(conn) << std::endl;
        return;
    }

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);
    };

    updateLayout();

    // Display issued books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the viewIssuedBookGUI function and go back to the previous menu
                }
            }
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw table headers
        for (int i = 0; i < 5; ++i) {
            window.draw(headers[i]);
        }

        // Draw issued books
        for (size_t i = 0; i < issuedBooks.size(); ++i) {
            for (size_t j = 0; j < issuedBooks[i].size(); ++j) {
                sf::Text cellText;
                cellText.setFont(font);
                cellText.setString(issuedBooks[i][j]);
                cellText.setCharacterSize(20);
                cellText.setFillColor(sf::Color::White);
                cellText.setPosition(50 + j * 150, 100 + i * 30);
                window.draw(cellText);
            }
        }

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}

void registerStudentGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input fields setup
    sf::RectangleShape nameBox(sf::Vector2f(400, 50));
    nameBox.setFillColor(sf::Color::White);
    nameBox.setPosition(50, 100);

    sf::Text nameText;
    nameText.setFont(font);
    nameText.setCharacterSize(24);
    nameText.setFillColor(sf::Color::Black);
    nameText.setPosition(60, 110);

    std::string nameInput;

    sf::RectangleShape emailBox(sf::Vector2f(400, 50));
    emailBox.setFillColor(sf::Color::White);
    emailBox.setPosition(50, 200);

    sf::Text emailText;
    emailText.setFont(font);
    emailText.setCharacterSize(24);
    emailText.setFillColor(sf::Color::Black);
    emailText.setPosition(60, 210);

    std::string emailInput;

    sf::RectangleShape studentIdBox(sf::Vector2f(400, 50));
    studentIdBox.setFillColor(sf::Color::White);
    studentIdBox.setPosition(50, 300);

    sf::Text studentIdText;
    studentIdText.setFont(font);
    studentIdText.setCharacterSize(24);
    studentIdText.setFillColor(sf::Color::Black);
    studentIdText.setPosition(60, 310);

    std::string studentIdInput;

    // Labels for input fields
    sf::Text nameLabel("Name:", font, 24);
    nameLabel.setFillColor(sf::Color::White);
    nameLabel.setPosition(50, 70);

    sf::Text emailLabel("Email:", font, 24);
    emailLabel.setFillColor(sf::Color::White);
    emailLabel.setPosition(50, 170);

    sf::Text studentIdLabel("Student ID:", font, 24);
    studentIdLabel.setFillColor(sf::Color::White);
    studentIdLabel.setPosition(50, 270);

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Register button setup
    sf::RectangleShape registerButton(sf::Vector2f(150, 50));
    registerButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    registerButton.setOutlineColor(sf::Color::White);
    registerButton.setOutlineThickness(1);
    registerButton.setPosition(window.getSize().x - 200, window.getSize().y - 100);

    sf::Text registerButtonText("Register", font, 24);
    registerButtonText.setFillColor(sf::Color::White);
    registerButtonText.setPosition(window.getSize().x - 170, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 400);

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 410);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the register button and its text
        registerButton.setPosition(windowSize.x - 200, windowSize.y - 100);
        registerButtonText.setPosition(windowSize.x - 170, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 400);
        messageText.setPosition(60, 410);
    };

    updateLayout();

    // Display books in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (nameBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!nameInput.empty()) {
                            nameInput.pop_back();
                        }
                    } else if (emailBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!emailInput.empty()) {
                            emailInput.pop_back();
                        }
                    } else if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!studentIdInput.empty()) {
                            studentIdInput.pop_back();
                        }
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    char enteredChar = static_cast<char>(event.text.unicode);
                    if (nameBox.getGlobalBounds().contains(cursor.getPosition())) {
                        nameInput += enteredChar;
                    } else if (emailBox.getGlobalBounds().contains(cursor.getPosition())) {
                        emailInput += enteredChar;
                    } else if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        studentIdInput += enteredChar;
                    }
                }
                nameText.setString(nameInput);
                emailText.setString(emailInput);
                studentIdText.setString(studentIdInput);
                cursor.setPosition(60 + (nameBox.getGlobalBounds().contains(cursor.getPosition()) ? nameText.getLocalBounds().width :
                emailBox.getGlobalBounds().contains(cursor.getPosition()) ? emailText.getLocalBounds().width : studentIdText.getLocalBounds().width) + 5, cursor.getPosition().y);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (registerButton.getGlobalBounds().contains(mousePos)) {
                    // Execute register student query
                    int studentId = std::stoi(studentIdInput);
                    std::string query = "INSERT INTO students (student_id, name, email) VALUES (" + std::to_string(studentId) + ", '" + nameInput + "', '" + emailInput + "');";

                    if (mysql_query(conn, query.c_str()) == 0) {
                        messageText.setFillColor(sf::Color::Green);
                        messageText.setString("Student registered successfully!");
                    } else {
                        messageText.setString("Cannot register the student: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the registerStudentGUI function and go back to the previous menu
                }

                // Move cursor to the clicked input box
                if (nameBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + nameText.getLocalBounds().width + 5, 110);
                } else if (emailBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + emailText.getLocalBounds().width + 5, 210);
                } else if (studentIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + studentIdText.getLocalBounds().width + 5, 310);
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input fields, labels, and message box
        window.draw(nameLabel);
        window.draw(nameBox);
        window.draw(nameText);
        window.draw(emailLabel);
        window.draw(emailBox);
        window.draw(emailText);
        window.draw(studentIdLabel);
        window.draw(studentIdBox);
        window.draw(studentIdText);
        window.draw(messageBox);
        window.draw(messageText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the register button
        window.draw(registerButton);
        window.draw(registerButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}

void removeStudentGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input field setup
    sf::RectangleShape studentIdBox(sf::Vector2f(400, 50));
    studentIdBox.setFillColor(sf::Color::White);
    studentIdBox.setPosition(50, 100);

    sf::Text studentIdText;
    studentIdText.setFont(font);
    studentIdText.setCharacterSize(24);
    studentIdText.setFillColor(sf::Color::Black);
    studentIdText.setPosition(60, 110);

    std::string studentIdInput;

    // Label for input field
    sf::Text studentIdLabel("Student ID:", font, 24);
    studentIdLabel.setFillColor(sf::Color::White);
    studentIdLabel.setPosition(50, 70);

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Remove button setup
    sf::RectangleShape removeButton(sf::Vector2f(100, 50));
    removeButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    removeButton.setOutlineColor(sf::Color::White);
    removeButton.setOutlineThickness(1);
    removeButton.setPosition(window.getSize().x - 150, window.getSize().y - 100);

    sf::Text removeButtonText("Remove", font, 24);
    removeButtonText.setFillColor(sf::Color::White);
    removeButtonText.setPosition(window.getSize().x - 125, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 200);

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 210);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the remove button and its text
        removeButton.setPosition(windowSize.x - 150, windowSize.y - 100);
        removeButtonText.setPosition(windowSize.x - 125, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 200);
        messageText.setPosition(60, 210);
    };

    updateLayout();

    // Display in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (!studentIdInput.empty()) {
                        studentIdInput.pop_back();
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    char enteredChar = static_cast<char>(event.text.unicode);
                    studentIdInput += enteredChar;
                }
                studentIdText.setString(studentIdInput);
                cursor.setPosition(60 + studentIdText.getLocalBounds().width + 5, 110);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (removeButton.getGlobalBounds().contains(mousePos)) {
                    // Execute remove student query
                    int studentId = std::stoi(studentIdInput);
                    std::string query = "DELETE FROM students WHERE student_id = " + std::to_string(studentId) + ";";

                    if (mysql_query(conn, query.c_str()) == 0) {
                        messageText.setFillColor(sf::Color::Green);
                        messageText.setString("Student deleted successfully!");
                    } else {
                        messageText.setString("Failed to delete the student: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the removeStudentGUI function and go back to the previous menu
                }

                // Move cursor to the clicked input box
                if (studentIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + studentIdText.getLocalBounds().width + 5, 110);
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input field, label, and message box
        window.draw(studentIdLabel);
        window.draw(studentIdBox);
        window.draw(studentIdText);
        window.draw(messageBox);
        window.draw(messageText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the remove button
        window.draw(removeButton);
        window.draw(removeButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}

void viewStudentGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Table headers setup
    sf::Text headers[3];
    std::string headerLabels[3] = {"Student ID", "Name", "Email"};
    for (int i = 0; i < 3; ++i) {
        headers[i].setFont(font);
        headers[i].setString(headerLabels[i]);
        headers[i].setCharacterSize(24);
        headers[i].setFillColor(sf::Color::White);
        headers[i].setPosition(50 + i * 200, 50);
    }

    // Query to fetch all students
    std::string viewStudents = "SELECT * FROM students";
    MYSQL_RES* res;
    MYSQL_ROW row;
    std::vector<std::vector<std::string>> students;

    // Execute the query
    if (mysql_query(conn, viewStudents.c_str()) == 0) {
        res = mysql_store_result(conn);

        if (res != nullptr) {
            // Fetch all rows and store them in students
            while ((row = mysql_fetch_row(res))) {
                std::vector<std::string> studentRow;
                for (int i = 0; i < 3; ++i) {
                    studentRow.push_back(row[i] ? row[i] : "NULL");
                }
                students.push_back(studentRow);
            }
            mysql_free_result(res);
        } else {
            std::cerr << "Failed to fetch result set: " << mysql_error(conn) << std::endl;
            return;
        }
    } else {
        std::cerr << "Failed to execute query: " << mysql_error(conn) << std::endl;
        return;
    }

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);
    };

    updateLayout();

    // Display students in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the viewStudentGUI function and go back to the previous menu
                }
            }
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw table headers
        for (int i = 0; i < 3; ++i) {
            window.draw(headers[i]);
        }

        // Draw students
        for (size_t i = 0; i < students.size(); ++i) {
            for (size_t j = 0; j < students[i].size(); ++j) {
                sf::Text cellText;
                cellText.setFont(font);
                cellText.setString(students[i][j]);
                cellText.setCharacterSize(20);
                cellText.setFillColor(sf::Color::White);
                cellText.setPosition(50 + j * 200, 100 + i * 30);
                window.draw(cellText);
            }
        }

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}


void modifyStudentGUI(sf::RenderWindow& window, sf::Font& font, sf::Texture& backgroundTexture, MYSQL* conn) {
    // Set up background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Input field setup
    sf::RectangleShape studentIdBox(sf::Vector2f(400, 50));
    studentIdBox.setFillColor(sf::Color::White);
    studentIdBox.setPosition(50, 100);

    sf::Text studentIdText;
    studentIdText.setFont(font);
    studentIdText.setCharacterSize(24);
    studentIdText.setFillColor(sf::Color::Black);
    studentIdText.setPosition(60, 110);

    std::string studentIdInput;

    // Label for input field
    sf::Text studentIdLabel("Student ID:", font, 24);
    studentIdLabel.setFillColor(sf::Color::White);
    studentIdLabel.setPosition(50, 70);

    // Options for modifying fields
    sf::Text optionsText[3];
    std::string optionLabels[3] = {"Modify Student ID", "Modify Name", "Modify Email"};
    int selectedOption = -1; // No option selected initially
    for (int i = 0; i < 3; ++i) {
        optionsText[i].setFont(font);
        optionsText[i].setString(optionLabels[i]);
        optionsText[i].setCharacterSize(24);
        optionsText[i].setFillColor(sf::Color::White);
        optionsText[i].setPosition(50, 200 + i * 60);
    }

    // Input field for new value
    sf::RectangleShape newValueBox(sf::Vector2f(400, 50));
    newValueBox.setFillColor(sf::Color::White);
    newValueBox.setPosition(50, 400);

    sf::Text newValueText;
    newValueText.setFont(font);
    newValueText.setCharacterSize(24);
    newValueText.setFillColor(sf::Color::Black);
    newValueText.setPosition(60, 410);

    std::string newValueInput;

    // Label for new value input field
    sf::Text newValueLabel("New Value:", font, 24);
    newValueLabel.setFillColor(sf::Color::White);
    newValueLabel.setPosition(50, 370);

    // Cursor setup
    sf::RectangleShape cursor(sf::Vector2f(2, 24));
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(60, 110);
    bool showCursor = true;
    sf::Clock cursorClock;

    // Save button setup
    sf::RectangleShape saveButton(sf::Vector2f(100, 50));
    saveButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    saveButton.setOutlineColor(sf::Color::White);
    saveButton.setOutlineThickness(1);
    saveButton.setPosition(window.getSize().x - 150, window.getSize().y - 100);

    sf::Text saveButtonText("Save", font, 24);
    saveButtonText.setFillColor(sf::Color::White);
    saveButtonText.setPosition(window.getSize().x - 125, window.getSize().y - 90);

    // Back button setup
    sf::RectangleShape backButton(sf::Vector2f(100, 50));
    backButton.setFillColor(sf::Color(93, 64, 55)); // Brown color
    backButton.setOutlineColor(sf::Color::White);
    backButton.setOutlineThickness(1);
    backButton.setPosition(50, window.getSize().y - 100);

    sf::Text backButtonText("Back", font, 24);
    backButtonText.setFillColor(sf::Color::White);
    backButtonText.setPosition(65, window.getSize().y - 90);

    // Message box setup
    sf::RectangleShape messageBox(sf::Vector2f(600, 50));
    messageBox.setFillColor(sf::Color::White);
    messageBox.setPosition(50, 500);

    sf::Text messageText;
    messageText.setFont(font);
    messageText.setCharacterSize(24);
    messageText.setFillColor(sf::Color::Red);
    messageText.setPosition(60, 510);

    // Function to update layout dynamically
    auto updateLayout = [&]() {
        // Responsive background scaling (ensure full coverage)
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);
        backgroundSprite.setPosition(0, 0);

        // Update the position of the back button and its text
        backButton.setPosition(50, windowSize.y - 100);
        backButtonText.setPosition(65, windowSize.y - 90);

        // Update the position of the save button and its text
        saveButton.setPosition(windowSize.x - 150, windowSize.y - 100);
        saveButtonText.setPosition(windowSize.x - 125, windowSize.y - 90);

        // Update the position of the message box and text
        messageBox.setPosition(50, 500);
        messageText.setPosition(60, 510);
    };

    updateLayout();

    // Display in SFML window
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                updateLayout();
            }

            // Handle text input
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') { // Handle backspace
                    if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!studentIdInput.empty()) {
                            studentIdInput.pop_back();
                        }
                    } else if (newValueBox.getGlobalBounds().contains(cursor.getPosition())) {
                        if (!newValueInput.empty()) {
                            newValueInput.pop_back();
                        }
                    }
                } else if (event.text.unicode < 128 && event.text.unicode != '\b') {
                    char enteredChar = static_cast<char>(event.text.unicode);
                    if (studentIdBox.getGlobalBounds().contains(cursor.getPosition())) {
                        studentIdInput += enteredChar;
                    } else if (newValueBox.getGlobalBounds().contains(cursor.getPosition())) {
                        newValueInput += enteredChar;
                    }
                }
                studentIdText.setString(studentIdInput);
                newValueText.setString(newValueInput);
                cursor.setPosition(60 + (studentIdBox.getGlobalBounds().contains(cursor.getPosition())
                                         ? studentIdText.getLocalBounds().width
                                         : newValueText.getLocalBounds().width) + 5, cursor.getPosition().y);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (saveButton.getGlobalBounds().contains(mousePos)) {
                    // Execute modify student query
                    int studentId = std::stoi(studentIdInput);
                    std::string query = "SELECT * FROM students WHERE student_id = " + std::to_string(studentId) + ";";

                    if (mysql_query(conn, query.c_str()) == 0) {
                        MYSQL_RES* res = mysql_store_result(conn);
                        if (res != nullptr) {
                            if (mysql_num_rows(res) == 0) {
                                messageText.setFillColor(sf::Color::Red);
                                messageText.setString("Student not found. Contact Librarian for registration.");
                                mysql_free_result(res);
                            } else {
                                mysql_free_result(res);

                                // Check which option is selected
                                if (selectedOption == 0) { // Modify Student ID
                                    int newStudentId = std::stoi(newValueInput);
                                    std::string idQuery = "UPDATE students SET student_id = " + std::to_string(newStudentId) +
                                                         " WHERE student_id = " + std::to_string(studentId) + ";";
                                    if (mysql_query(conn, idQuery.c_str()) == 0) {
                                        messageText.setFillColor(sf::Color::Green);
                                        messageText.setString("Student ID updated successfully!");
                                    } else {
                                        messageText.setFillColor(sf::Color::Red);
                                        messageText.setString("Failed to update Student ID: " + std::string(mysql_error(conn)));
                                    }
                                } else if (selectedOption == 1) { // Modify Name
                                    std::string nameQuery = "UPDATE students SET name = '" + newValueInput +
                                                            "' WHERE student_id = " + std::to_string(studentId) + ";";
                                    if (mysql_query(conn, nameQuery.c_str()) == 0) {
                                        messageText.setFillColor(sf::Color::Green);
                                        messageText.setString("Name updated successfully!");
                                    } else {
                                        messageText.setFillColor(sf::Color::Red);
                                        messageText.setString("Failed to update Name: " + std::string(mysql_error(conn)));
                                    }
                                } else if (selectedOption == 2) { // Modify Email
                                    std::string emailQuery = "UPDATE students SET email = '" + newValueInput +
                                                            "' WHERE student_id = " + std::to_string(studentId) + ";";
                                    if (mysql_query(conn, emailQuery.c_str()) == 0) {
                                        messageText.setFillColor(sf::Color::Green);
                                        messageText.setString("Email updated successfully!");
                                    } else {
                                        messageText.setFillColor(sf::Color::Red);
                                        messageText.setString("Failed to update Email: " + std::string(mysql_error(conn)));
                                    }
                                } else {
                                    messageText.setFillColor(sf::Color::Red);
                                    messageText.setString("Please select a field to modify.");
                                }
                            }
                        } else {
                            messageText.setFillColor(sf::Color::Red);
                            messageText.setString("Failed to fetch the student: " + std::string(mysql_error(conn)));
                        }
                    } else {
                        messageText.setFillColor(sf::Color::Red);
                        messageText.setString("Failed to execute query: " + std::string(mysql_error(conn)));
                    }
                }

                if (backButton.getGlobalBounds().contains(mousePos)) {
                    return; // Exit the modifyStudentGUI function and go back to the previous menu
                }

                // Check if an option is clicked
                for (int i = 0; i < 3; ++i) {
                    if (optionsText[i].getGlobalBounds().contains(mousePos)) {
                        selectedOption = i;
                        for (int j = 0; j < 3; ++j) {
                            optionsText[j].setFillColor(j == i ? sf::Color::Green : sf::Color::White);
                        }
                    }
                }

                // Move cursor to the clicked input box
                if (studentIdBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + studentIdText.getLocalBounds().width + 5, 110);
                } else if (newValueBox.getGlobalBounds().contains(mousePos)) {
                    cursor.setPosition(60 + newValueText.getLocalBounds().width + 5, 410);
                }
            }
        }

        // Blink cursor
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        window.clear();

        // Draw the background
        window.draw(backgroundSprite);

        // Draw input fields, labels, options, and message box
        window.draw(studentIdLabel);
        window.draw(studentIdBox);
        window.draw(studentIdText);
        window.draw(newValueLabel);
        window.draw(newValueBox);
        window.draw(newValueText);
        for (int i = 0; i < 3; ++i) {
            window.draw(optionsText[i]);
        }
        window.draw(messageBox);
        window.draw(messageText);

        // Draw cursor if visible
        if (showCursor) {
            window.draw(cursor);
        }

        // Draw the save button
        window.draw(saveButton);
        window.draw(saveButtonText);

        // Draw the back button
        window.draw(backButton);
        window.draw(backButtonText);

        window.display();
    }
}



void librarianMenu(sf::RenderWindow& window, sf::Font& font, sf::Sprite& background,MYSQL* conn) {

    sf::Texture modifyTexture;
    if (!modifyTexture.loadFromFile("modify.jpg")) {
        std::cerr << "Error: Could not load 'modify.png'. Ensure the file exists in the correct directory.\n";
        return;
    }


    sf::Texture deleteTexture;
    if (!deleteTexture.loadFromFile("delete.png")) {
        std::cerr << "Error: Could not load 'delete.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture solidTexture;
    if (!solidTexture.loadFromFile("solid.png")) {
        std::cerr << "Error: Could not load 'solid.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture addTexture;
    if (!addTexture.loadFromFile("add.png")) {
        std::cerr << "Error: Could not load 'add.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture viewIssuedTexture;
    if (!viewIssuedTexture.loadFromFile("viewIssued.jpg")) {
        std::cerr << "Error: Could not load 'viewIssued.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture registerStudent;
    if (!registerStudent.loadFromFile("student.png")) {
        std::cerr << "Error: Could not load 'modify.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture removeStudent;
    if (!removeStudent.loadFromFile("removeStudent.png")) {
        std::cerr << "Error: Could not load 'removeStudent.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture viewStudent;
    if (!viewStudent.loadFromFile("viewStudent.png")) {
        std::cerr << "Error: Could not load 'viewStudent.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    sf::Texture modifyStudent;
    if (!modifyStudent.loadFromFile("modifyStudent.png")) {
        std::cerr << "Error: Could not load 'modifyStudent.png'. Ensure the file exists in the correct directory.\n";
        return;
    }

    // Librarian Menu Setup
    std::vector<sf::RectangleShape> librarianMenuButtons;
    std::vector<sf::Text> librarianMenuTexts;

    float buttonWidth = 200;
    float buttonHeight = 50;

    std::vector<std::string> librarianMenuLabels = {
        "Add Books", "Remove Books", "View Books", "Modify Books", "View Issued Books",
        "Register Students", "Remove Students", "View Students", "Modify Students", "Exit"
    };

    for (size_t i = 0; i < librarianMenuLabels.size(); ++i) {
    // Create button shape
    sf::RectangleShape button(sf::Vector2f(buttonWidth, buttonHeight));
    float xPos = (i < 5) ? (window.getSize().x / 4 - buttonWidth / 2) : (3 * window.getSize().x / 4 - buttonWidth / 2);
    float yPos = 200 + (i % 5) * (buttonHeight + 20);
    button.setPosition(xPos, yPos);
    button.setFillColor(sf::Color(93, 64, 55)); // Default brown color

    // Set outline color and thickness
    button.setOutlineColor(sf::Color::White);
    button.setOutlineThickness(2);

    librarianMenuButtons.push_back(button);

    // Create button text
    sf::Text text;
    text.setFont(font);
    text.setString(librarianMenuLabels[i]);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::White);
    text.setPosition(
        xPos + (buttonWidth - text.getLocalBounds().width) / 2,
        yPos + (buttonHeight - text.getLocalBounds().height) / 2);
    librarianMenuTexts.push_back(text);
}

    // Loop to handle Librarian Menu events
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                for (size_t i = 0; i < librarianMenuButtons.size(); ++i) {
                     if (librarianMenuButtons[i].getGlobalBounds().contains(mousePos)) {
                        if (librarianMenuTexts[i].getString() == "Exit") {
                            return; // Exit librarian menu
                        } else if (librarianMenuTexts[i].getString() == "Add Books") {
                            addBookGUI(window, font, addTexture, conn); // Call the addBookGUI function
                        } else if (librarianMenuTexts[i].getString() == "View Books") {
                            viewBooksGUI(window, font, solidTexture, conn); // Call the viewBooksGUI function
                        } else if (librarianMenuTexts[i].getString() == "Remove Books") {
                            deleteBookGUI(window, font, deleteTexture, conn); // Call the searchBookGUI function
                        } else if (librarianMenuTexts[i].getString() == "Modify Books") {
                            modifyBookGUI(window, font, modifyTexture, conn); // Call the issueBookGUI function
                        } else if (librarianMenuTexts[i].getString() == "View Issued Books") {
                            viewIssuedBookGUI(window, font, viewIssuedTexture, conn); // Call the returnBookGUI function
                        } else if(librarianMenuTexts[i].getString() == "Register Students") {
                            registerStudentGUI(window,font,registerStudent,conn);
                        }else if(librarianMenuTexts[i].getString() == "Remove Students"){
                            removeStudentGUI(window,font,removeStudent,conn);
                        }else if(librarianMenuTexts[i].getString() == "View Students"){
                            viewStudentGUI(window,font,viewStudent,conn);
                        }else if(librarianMenuTexts[i].getString() == "Modify Students")
                            modifyStudentGUI(window,font,modifyStudent,conn);
                        else {
                            std::cout << librarianMenuTexts[i].getString().toAnsiString() << " clicked!\n";
                        }
                    }
                }
            }
        }

        // Rendering
        window.clear();

        // Draw the background
        window.draw(background);

        // Draw librarian menu buttons
        for (size_t i = 0; i < librarianMenuButtons.size(); ++i) {
            window.draw(librarianMenuButtons[i]);
            window.draw(librarianMenuTexts[i]);
        }

        window.display();
    }
}

int main() {
    Database db;

    MYSQL* conn = db.getConnection();

    // Create the SFML window
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Library Management System", sf::Style::Default);
    window.setFramerateLimit(60);

    // Load background texture
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("background.jpg")) {
        std::cerr << "Error: Could not load 'background.jpg'. Ensure the file exists in the correct directory.\n";
        return -1;
    }


    // Load font
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Error: Could not load 'arial.ttf'. Ensure the file exists in the correct directory.\n";
        return -1;
    }

    // Background sprite
    sf::Sprite backgroundSprite(backgroundTexture);

    // Heading
    sf::Text heading("Library Management System", font, 50);
    heading.setFillColor(sf::Color(255, 215, 0)); // Gold color
    heading.setStyle(sf::Text::Bold);
    heading.setOutlineColor(sf::Color::White);
    heading.setOutlineThickness(0.7f);

    // Buttons
    sf::RectangleShape studentButton(sf::Vector2f(250, 50));
    sf::RectangleShape librarianButton(sf::Vector2f(250, 50));
    sf::RectangleShape exitButton(sf::Vector2f(250, 50));

    // Button Styling
    studentButton.setFillColor(sf::Color(93, 64, 55));   // Brown
    studentButton.setOutlineColor(sf::Color::White);
    studentButton.setOutlineThickness(1);

    librarianButton.setFillColor(sf::Color(93, 64, 55)); // Brown
    librarianButton.setOutlineColor(sf::Color::White);
    librarianButton.setOutlineThickness(1);

    exitButton.setFillColor(sf::Color(93, 64, 55));      // Brown
    exitButton.setOutlineColor(sf::Color::White);
    exitButton.setOutlineThickness(1);

    sf::Text studentText("Student", font, 24);
    sf::Text librarianText("Librarian", font, 24);
    sf::Text exitText("Exit", font, 24);

    // Text Styling
    studentText.setFillColor(sf::Color::White);  // White text color
    librarianText.setFillColor(sf::Color::White);
    exitText.setFillColor(sf::Color::White);

    // Function to update layout dynamically
    auto updateLayout = [&](sf::Vector2u windowSize) {
       // Responsive background scaling (ensure full coverage)
        sf::Vector2u textureSize = backgroundTexture.getSize();
        float scaleX = static_cast<float>(windowSize.x) / textureSize.x;
        float scaleY = static_cast<float>(windowSize.y) / textureSize.y;
        float scale = std::max(scaleX, scaleY); // Use max to cover the entire area

        backgroundSprite.setScale(scale, scale);

        // Align background to top-left corner
        backgroundSprite.setPosition(0, 0);

        // Center heading
        heading.setCharacterSize(windowSize.x / 20); // Adjust text size dynamically
        sf::FloatRect headingBounds = heading.getLocalBounds();
        heading.setOrigin(headingBounds.width / 2, headingBounds.height / 2);
        heading.setPosition(windowSize.x / 2, windowSize.y / 10);

        // Buttons
        float buttonWidth = windowSize.x / 4;
        float buttonHeight = windowSize.y / 12;
        float buttonSpacing = buttonHeight / 2;

        studentButton.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        librarianButton.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        exitButton.setSize(sf::Vector2f(buttonWidth, buttonHeight));

        studentButton.setPosition(windowSize.x / 2 - buttonWidth / 2, windowSize.y / 2 - 1.5 * (buttonHeight + buttonSpacing));
        librarianButton.setPosition(windowSize.x / 2 - buttonWidth / 2, windowSize.y / 2 - 0.5 * (buttonHeight + buttonSpacing));
        exitButton.setPosition(windowSize.x / 2 - buttonWidth / 2, windowSize.y / 2 + 0.5 * (buttonHeight + buttonSpacing));

        // Button text
        auto centerText = [&](sf::Text &text, const sf::RectangleShape &button) {
            text.setCharacterSize(windowSize.y / 30); // Adjust text size dynamically
            sf::FloatRect textBounds = text.getLocalBounds();
            text.setOrigin(textBounds.width / 2, textBounds.height / 2);
            text.setPosition(
                button.getPosition().x + button.getSize().x / 2,
                button.getPosition().y + button.getSize().y / 2 - textBounds.height / 2
            );
        };

        centerText(studentText, studentButton);
        centerText(librarianText, librarianButton);
        centerText(exitText, exitButton);
    };

    // Initialize layout
    updateLayout(window.getSize());

    // Event loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Handle window resize
            if (event.type == sf::Event::Resized) {
                sf::Vector2u newSize(event.size.width, event.size.height);
                window.setView(sf::View(sf::FloatRect(0, 0, newSize.x, newSize.y)));
                updateLayout(newSize);
            }

            // Handle mouse click
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));

                if (studentButton.getGlobalBounds().contains(mousePos)) {
                    studentMenu(window, font, backgroundSprite,conn); // Pass background to studentMenu
                } else if (librarianButton.getGlobalBounds().contains(mousePos)) {
                    librarianMenu(window, font, backgroundSprite,conn); // Pass background to librarianMenu
                } else if (exitButton.getGlobalBounds().contains(mousePos)) {
                    window.close();
                }
            }
        }

        // Render
        window.clear();
        window.draw(backgroundSprite);
        window.draw(heading);
        window.draw(studentButton);
        window.draw(studentText);
        window.draw(librarianButton);
        window.draw(librarianText);
        window.draw(exitButton);
        window.draw(exitText);
        window.display();
    }

    return 0;
}
