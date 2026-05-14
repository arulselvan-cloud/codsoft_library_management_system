/*
 * ============================================================
 *   LIBRARY MANAGEMENT SYSTEM — C++
 *   Features: Book DB, Search, Checkout, Return, Fine Calc
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <stdexcept>
#include <limits>

using namespace std;

// ─────────────────────────────────────────────
// UTILITY: Get today's date as "YYYY-MM-DD"
// ─────────────────────────────────────────────
string today() {
    time_t t = time(nullptr);
    tm *lt = localtime(&t);
    ostringstream oss;
    oss << (1900 + lt->tm_year) << "-"
        << setw(2) << setfill('0') << (1 + lt->tm_mon) << "-"
        << setw(2) << setfill('0') << lt->tm_mday;
    return oss.str();
}

// Days between two "YYYY-MM-DD" strings (b − a)
long daysBetween(const string &a, const string &b) {
    auto toTm = [](const string &s) {
        tm t{};
        istringstream ss(s);
        char dash;
        ss >> t.tm_year >> dash >> t.tm_mon >> dash >> t.tm_mday;
        t.tm_year -= 1900;
        t.tm_mon  -= 1;
        t.tm_isdst = -1;
        return mktime(&t);
    };
    double diff = difftime(toTm(b), toTm(a));
    return static_cast<long>(diff / 86400.0);
}

// ─────────────────────────────────────────────
// DATA STRUCTURES
// ─────────────────────────────────────────────
struct Book {
    string isbn;
    string title;
    string author;
    int    totalCopies   = 1;
    int    availableCopies = 1;
};

struct Borrower {
    string id;
    string name;
    string contact;
};

struct Transaction {
    int    txnId;
    string borrowerId;
    string isbn;
    string checkoutDate;
    string dueDate;          // checkoutDate + 14 days
    string returnDate;       // empty = not yet returned
    double fine = 0.0;
};

// ─────────────────────────────────────────────
// IN-MEMORY DATABASE
// ─────────────────────────────────────────────
class Library {
    map<string, Book>       books;       // isbn → Book
    map<string, Borrower>   borrowers;   // id   → Borrower
    vector<Transaction>     transactions;
    int nextTxnId = 1001;

    // Fine: ₹2 per overdue day
    static constexpr double FINE_PER_DAY     = 2.0;
    static constexpr int    LOAN_PERIOD_DAYS = 14;

    // ── Persistence file names ──────────────────
    const string BOOKS_FILE = "books.csv";
    const string BORR_FILE  = "borrowers.csv";
    const string TXN_FILE   = "transactions.csv";

public:
    Library()  { load(); }
    ~Library() { save(); }

    // ── Helpers ─────────────────────────────────
    string addDays(const string &date, int days) {
        tm t{};
        istringstream ss(date);
        char dash;
        ss >> t.tm_year >> dash >> t.tm_mon >> dash >> t.tm_mday;
        t.tm_year -= 1900; t.tm_mon -= 1; t.tm_isdst = -1;
        time_t raw = mktime(&t);
        raw += static_cast<time_t>(days) * 86400;
        tm *nt = localtime(&raw);
        ostringstream oss;
        oss << (1900 + nt->tm_year) << "-"
            << setw(2) << setfill('0') << (1 + nt->tm_mon) << "-"
            << setw(2) << setfill('0') << nt->tm_mday;
        return oss.str();
    }

    string trim(const string &s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        return (a == string::npos) ? "" : s.substr(a, b - a + 1);
    }

    // ── CSV Persistence ──────────────────────────
    void save() {
        // books
        ofstream bf(BOOKS_FILE);
        bf << "isbn,title,author,totalCopies,availableCopies\n";
        for (auto &[k, bk] : books)
            bf << bk.isbn << "," << bk.title << "," << bk.author
               << "," << bk.totalCopies << "," << bk.availableCopies << "\n";

        // borrowers
        ofstream brf(BORR_FILE);
        brf << "id,name,contact\n";
        for (auto &[k, br] : borrowers)
            brf << br.id << "," << br.name << "," << br.contact << "\n";

        // transactions
        ofstream tf(TXN_FILE);
        tf << "txnId,borrowerId,isbn,checkoutDate,dueDate,returnDate,fine\n";
        for (auto &tx : transactions)
            tf << tx.txnId << "," << tx.borrowerId << "," << tx.isbn
               << "," << tx.checkoutDate << "," << tx.dueDate
               << "," << tx.returnDate << "," << fixed << setprecision(2)
               << tx.fine << "\n";
    }

    void load() {
        // books
        ifstream bf(BOOKS_FILE);
        if (bf.is_open()) {
            string line; getline(bf, line); // header
            while (getline(bf, line)) {
                if (line.empty()) continue;
                istringstream ss(line);
                Book bk;
                string tc, ac;
                getline(ss, bk.isbn,  ',');
                getline(ss, bk.title, ',');
                getline(ss, bk.author,',');
                getline(ss, tc, ',');
                getline(ss, ac, ',');
                bk.totalCopies     = stoi(tc);
                bk.availableCopies = stoi(ac);
                books[bk.isbn] = bk;
            }
        }
        // borrowers
        ifstream brf(BORR_FILE);
        if (brf.is_open()) {
            string line; getline(brf, line);
            while (getline(brf, line)) {
                if (line.empty()) continue;
                istringstream ss(line);
                Borrower br;
                getline(ss, br.id,      ',');
                getline(ss, br.name,    ',');
                getline(ss, br.contact, ',');
                borrowers[br.id] = br;
            }
        }
        // transactions
        ifstream tf(TXN_FILE);
        if (tf.is_open()) {
            string line; getline(tf, line);
            while (getline(tf, line)) {
                if (line.empty()) continue;
                istringstream ss(line);
                Transaction tx;
                string tid, fine;
                getline(ss, tid,           ',');
                getline(ss, tx.borrowerId, ',');
                getline(ss, tx.isbn,       ',');
                getline(ss, tx.checkoutDate,',');
                getline(ss, tx.dueDate,    ',');
                getline(ss, tx.returnDate, ',');
                getline(ss, fine,          ',');
                tx.txnId = stoi(tid);
                tx.fine  = stod(fine);
                transactions.push_back(tx);
                if (tx.txnId >= nextTxnId) nextTxnId = tx.txnId + 1;
            }
        }
    }

    // ═══════════════════════════════════════════
    //  BOOK OPERATIONS
    // ═══════════════════════════════════════════
    void addBook(const string &isbn, const string &title,
                 const string &author, int copies) {
        if (books.count(isbn)) {
            books[isbn].totalCopies     += copies;
            books[isbn].availableCopies += copies;
            cout << "  [+] Updated existing book. Total copies: "
                 << books[isbn].totalCopies << "\n";
        } else {
            books[isbn] = {isbn, title, author, copies, copies};
            cout << "  [+] Book added successfully.\n";
        }
    }

    void listBooks() {
        if (books.empty()) { cout << "  No books in database.\n"; return; }
        cout << "\n  " << string(72, '─') << "\n";
        cout << "  " << left
             << setw(14) << "ISBN"
             << setw(28) << "Title"
             << setw(18) << "Author"
             << setw(6)  << "Avail"
             << setw(6)  << "Total" << "\n";
        cout << "  " << string(72, '─') << "\n";
        for (auto &[k, bk] : books)
            cout << "  " << left
                 << setw(14) << bk.isbn
                 << setw(28) << bk.title.substr(0, 26)
                 << setw(18) << bk.author.substr(0, 16)
                 << setw(6)  << bk.availableCopies
                 << setw(6)  << bk.totalCopies << "\n";
        cout << "  " << string(72, '─') << "\n";
    }

    void searchBooks(const string &query) {
        string q = query;
        transform(q.begin(), q.end(), q.begin(), ::tolower);
        bool found = false;
        cout << "\n  Search results for \"" << query << "\":\n";
        cout << "  " << string(72, '─') << "\n";
        for (auto &[k, bk] : books) {
            string title  = bk.title,  author = bk.author, isbn = bk.isbn;
            transform(title.begin(),  title.end(),  title.begin(),  ::tolower);
            transform(author.begin(), author.end(), author.begin(), ::tolower);
            transform(isbn.begin(),   isbn.end(),   isbn.begin(),   ::tolower);
            if (title.find(q) != string::npos ||
                author.find(q)!= string::npos ||
                isbn.find(q)  != string::npos) {
                cout << "  ISBN   : " << bk.isbn   << "\n"
                     << "  Title  : " << bk.title  << "\n"
                     << "  Author : " << bk.author << "\n"
                     << "  Copies : " << bk.availableCopies
                     << " / " << bk.totalCopies << "\n";
                cout << "  " << string(72, '─') << "\n";
                found = true;
            }
        }
        if (!found) cout << "  No matching books found.\n";
    }

    // ═══════════════════════════════════════════
    //  BORROWER OPERATIONS
    // ═══════════════════════════════════════════
    void addBorrower(const string &id, const string &name, const string &contact) {
        if (borrowers.count(id)) {
            cout << "  [!] Borrower ID already exists.\n"; return;
        }
        borrowers[id] = {id, name, contact};
        cout << "  [+] Borrower registered successfully.\n";
    }

    void listBorrowers() {
        if (borrowers.empty()) { cout << "  No borrowers registered.\n"; return; }
        cout << "\n  " << string(55, '─') << "\n";
        cout << "  " << left
             << setw(10) << "ID"
             << setw(25) << "Name"
             << setw(20) << "Contact" << "\n";
        cout << "  " << string(55, '─') << "\n";
        for (auto &[k, br] : borrowers)
            cout << "  " << left
                 << setw(10) << br.id
                 << setw(25) << br.name
                 << setw(20) << br.contact << "\n";
        cout << "  " << string(55, '─') << "\n";
    }

    // ═══════════════════════════════════════════
    //  CHECKOUT
    // ═══════════════════════════════════════════
    void checkoutBook(const string &borrowerId, const string &isbn) {
        if (!borrowers.count(borrowerId)) {
            cout << "  [!] Borrower not found.\n"; return;
        }
        if (!books.count(isbn)) {
            cout << "  [!] Book not found.\n"; return;
        }
        if (books[isbn].availableCopies <= 0) {
            cout << "  [!] No copies available right now.\n"; return;
        }
        // Check if borrower already has this book
        for (auto &tx : transactions)
            if (tx.borrowerId == borrowerId && tx.isbn == isbn && tx.returnDate.empty()) {
                cout << "  [!] Borrower already has this book checked out.\n";
                return;
            }

        books[isbn].availableCopies--;
        string co  = today();
        string due = addDays(co, LOAN_PERIOD_DAYS);
        Transaction tx{nextTxnId++, borrowerId, isbn, co, due, "", 0.0};
        transactions.push_back(tx);
        cout << "  [✔] Checkout successful!\n"
             << "      TXN ID      : " << tx.txnId       << "\n"
             << "      Borrower    : " << borrowers[borrowerId].name << "\n"
             << "      Book        : " << books[isbn].title << "\n"
             << "      Checkout    : " << co  << "\n"
             << "      Due Date    : " << due << "\n";
    }

    // ═══════════════════════════════════════════
    //  RETURN + FINE CALCULATION
    // ═══════════════════════════════════════════
    void returnBook(int txnId) {
        for (auto &tx : transactions) {
            if (tx.txnId == txnId) {
                if (!tx.returnDate.empty()) {
                    cout << "  [!] This transaction is already closed.\n"; return;
                }
                tx.returnDate = today();
                long overdue  = daysBetween(tx.dueDate, tx.returnDate);
                tx.fine = (overdue > 0) ? overdue * FINE_PER_DAY : 0.0;
                books[tx.isbn].availableCopies++;

                cout << "  [✔] Book returned successfully!\n"
                     << "      TXN ID    : " << tx.txnId       << "\n"
                     << "      Book ISBN : " << tx.isbn         << "\n"
                     << "      Due Date  : " << tx.dueDate      << "\n"
                     << "      Returned  : " << tx.returnDate   << "\n";
                if (tx.fine > 0)
                    cout << "      *** FINE  : ₹" << fixed << setprecision(2)
                         << tx.fine << " (" << overdue << " day(s) overdue) ***\n";
                else
                    cout << "      Fine      : None (returned on time)\n";
                return;
            }
        }
        cout << "  [!] Transaction not found.\n";
    }

    // ═══════════════════════════════════════════
    //  REPORTS
    // ═══════════════════════════════════════════
    void activeLoans() {
        cout << "\n  ── Active Loans ──────────────────────────────────────────\n";
        bool any = false;
        string td = today();
        for (auto &tx : transactions) {
            if (!tx.returnDate.empty()) continue;
            any = true;
            long overdue = daysBetween(tx.dueDate, td);
            cout << "  TXN " << tx.txnId
                 << " | Borrower: " << setw(10) << left << tx.borrowerId
                 << " | ISBN: " << tx.isbn
                 << " | Due: " << tx.dueDate;
            if (overdue > 0)
                cout << " | OVERDUE " << overdue << " day(s)"
                     << " | Est. Fine: ₹" << fixed << setprecision(2)
                     << overdue * FINE_PER_DAY;
            cout << "\n";
        }
        if (!any) cout << "  No active loans.\n";
    }

    void transactionHistory() {
        cout << "\n  ── Transaction History ───────────────────────────────────\n";
        if (transactions.empty()) { cout << "  No transactions yet.\n"; return; }
        for (auto &tx : transactions) {
            cout << "  TXN " << setw(5) << left << tx.txnId
                 << " Borrower: " << setw(8)  << tx.borrowerId
                 << " ISBN: "     << setw(14) << tx.isbn
                 << " Out: "      << tx.checkoutDate
                 << " Due: "      << tx.dueDate
                 << " Ret: "      << (tx.returnDate.empty() ? "—         " : tx.returnDate)
                 << " Fine: ₹"    << fixed << setprecision(2) << tx.fine << "\n";
        }
    }

    void fineReport() {
        double total = 0;
        cout << "\n  ── Fine Report ───────────────────────────────────────────\n";
        for (auto &tx : transactions) {
            if (tx.fine > 0) {
                cout << "  TXN " << tx.txnId
                     << " | Borrower: " << tx.borrowerId
                     << " | ISBN: "     << tx.isbn
                     << " | Fine: ₹"   << fixed << setprecision(2) << tx.fine << "\n";
                total += tx.fine;
            }
        }
        cout << "  ──────────────────────────────────────────────────────────\n"
             << "  Total Fines Collected: ₹" << fixed << setprecision(2) << total << "\n";
    }
};

// ─────────────────────────────────────────────
// UI HELPERS
// ─────────────────────────────────────────────
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    cout << "\033[2J\033[H";
#endif
}

void pause() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

string prompt(const string &msg) {
    cout << "  " << msg;
    string s; getline(cin, s);
    return s;
}

int promptInt(const string &msg) {
    while (true) {
        cout << "  " << msg;
        string s; getline(cin, s);
        try { return stoi(s); }
        catch (...) { cout << "  [!] Please enter a valid number.\n"; }
    }
}

void banner() {
    cout << R"(
  ╔══════════════════════════════════════════════════════════╗
  ║          📚  LIBRARY MANAGEMENT SYSTEM  📚              ║
  ╚══════════════════════════════════════════════════════════╝
)" << "\n";
}

void mainMenu() {
    cout << "  ┌──────────────────────────────────────┐\n"
         << "  │            MAIN MENU                 │\n"
         << "  ├──────────────────────────────────────┤\n"
         << "  │  1. Book Management                  │\n"
         << "  │  2. Borrower Management              │\n"
         << "  │  3. Transactions                     │\n"
         << "  │  4. Reports                          │\n"
         << "  │  0. Exit                             │\n"
         << "  └──────────────────────────────────────┘\n"
         << "  Choice: ";
}

void bookMenu() {
    cout << "\n  ── Book Management ────────────────────\n"
         << "  1. Add Book\n"
         << "  2. List All Books\n"
         << "  3. Search Books\n"
         << "  0. Back\n"
         << "  Choice: ";
}

void borrowerMenu() {
    cout << "\n  ── Borrower Management ────────────────\n"
         << "  1. Register Borrower\n"
         << "  2. List Borrowers\n"
         << "  0. Back\n"
         << "  Choice: ";
}

void txnMenu() {
    cout << "\n  ── Transactions ───────────────────────\n"
         << "  1. Checkout Book\n"
         << "  2. Return Book\n"
         << "  3. Active Loans\n"
         << "  0. Back\n"
         << "  Choice: ";
}

void reportMenu() {
    cout << "\n  ── Reports ────────────────────────────\n"
         << "  1. Transaction History\n"
         << "  2. Fine Report\n"
         << "  0. Back\n"
         << "  Choice: ";
}

// ─────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────
int main() {
    Library lib;
    string choice;

    while (true) {
        clearScreen();
        banner();
        mainMenu();
        getline(cin, choice);

        if (choice == "1") {
            // Book Management
            while (true) {
                clearScreen(); banner();
                bookMenu();
                getline(cin, choice);
                if (choice == "0") break;
                if (choice == "1") {
                    cout << "\n  ── Add Book ───────────────────────────\n";
                    string isbn   = prompt("ISBN         : ");
                    string title  = prompt("Title        : ");
                    string author = prompt("Author       : ");
                    int    copies = promptInt("Copies       : ");
                    lib.addBook(isbn, title, author, copies);
                } else if (choice == "2") {
                    lib.listBooks();
                } else if (choice == "3") {
                    string q = prompt("Search (title/author/ISBN): ");
                    lib.searchBooks(q);
                }
                pause();
            }

        } else if (choice == "2") {
            // Borrower Management
            while (true) {
                clearScreen(); banner();
                borrowerMenu();
                getline(cin, choice);
                if (choice == "0") break;
                if (choice == "1") {
                    cout << "\n  ── Register Borrower ──────────────────\n";
                    string id      = prompt("Borrower ID  : ");
                    string name    = prompt("Name         : ");
                    string contact = prompt("Contact/Email: ");
                    lib.addBorrower(id, name, contact);
                } else if (choice == "2") {
                    lib.listBorrowers();
                }
                pause();
            }

        } else if (choice == "3") {
            // Transactions
            while (true) {
                clearScreen(); banner();
                txnMenu();
                getline(cin, choice);
                if (choice == "0") break;
                if (choice == "1") {
                    cout << "\n  ── Checkout Book ──────────────────────\n";
                    string bid  = prompt("Borrower ID  : ");
                    string isbn = prompt("ISBN         : ");
                    lib.checkoutBook(bid, isbn);
                } else if (choice == "2") {
                    cout << "\n  ── Return Book ────────────────────────\n";
                    int txnId = promptInt("Transaction ID: ");
                    lib.returnBook(txnId);
                } else if (choice == "3") {
                    lib.activeLoans();
                }
                pause();
            }

        } else if (choice == "4") {
            // Reports
            while (true) {
                clearScreen(); banner();
                reportMenu();
                getline(cin, choice);
                if (choice == "0") break;
                if (choice == "1") lib.transactionHistory();
                else if (choice == "2") lib.fineReport();
                pause();
            }

        } else if (choice == "0") {
            cout << "\n  Data saved. Goodbye!\n\n";
            break;
        }
    }
    return 0;
}
