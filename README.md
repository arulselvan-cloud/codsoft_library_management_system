# 📚 Library Management System — C++

A console-based Library Management System built in C++ that handles books, borrowers, and transactions with persistent CSV storage.

---

## 📋 Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Compilation & Run](#compilation--run)
- [Usage Guide](#usage-guide)
- [Data Storage](#data-storage)
- [Fine Calculation](#fine-calculation)
- [Sample Workflow](#sample-workflow)
- [Configuration](#configuration)

---

## ✅ Features

- **Book Database** — Add and manage books with ISBN, title, author, and copy count
- **Book Search** — Search by title, author, or ISBN (case-insensitive)
- **Book Checkout** — Check out books to registered borrowers with a 14-day loan period
- **Book Return** — Record returns and automatically update availability
- **Fine Calculation** — ₹2 per overdue day, calculated on return
- **Reports** — Active loans, transaction history, and fine summary
- **Persistence** — All data saved to CSV files and reloaded on next run

---

## 📁 Project Structure

```
library_management/
│
├── library_management.cpp   # Main source file (all-in-one)
├── books.csv                # Auto-generated: book records
├── borrowers.csv            # Auto-generated: borrower records
├── transactions.csv         # Auto-generated: transaction records
└── README.md
```

---

## ⚙️ Requirements

| Tool | Version |
|------|---------|
| C++ Compiler (g++ / clang++) | C++17 or later |
| OS | Windows / Linux / macOS |

No external libraries required — uses only the C++ Standard Library.

---

## 🚀 Compilation & Run

### Linux / macOS
```bash
g++ -std=c++17 -o library library_management.cpp
./library
```

### Windows (MinGW)
```bash
g++ -std=c++17 -o library.exe library_management.cpp
library.exe
```

### Windows (MSVC)
```bash
cl /std:c++17 library_management.cpp /Fe:library.exe
library.exe
```

---

## 🖥️ Usage Guide

### Main Menu
```
╔══════════════════════════════════════════════════════════╗
║          📚  LIBRARY MANAGEMENT SYSTEM  📚              ║
╚══════════════════════════════════════════════════════════╝

  1. Book Management
  2. Borrower Management
  3. Transactions
  4. Reports
  0. Exit
```

---

### 1. Book Management

| Option | Description |
|--------|-------------|
| Add Book | Enter ISBN, title, author, and number of copies |
| List All Books | Displays all books with availability count |
| Search Books | Search by any of: title, author, or ISBN |

> If you add a book with an existing ISBN, the copy count is incremented.

---

### 2. Borrower Management

| Option | Description |
|--------|-------------|
| Register Borrower | Enter a unique ID, name, and contact/email |
| List Borrowers | Displays all registered borrowers |

---

### 3. Transactions

| Option | Description |
|--------|-------------|
| Checkout Book | Enter Borrower ID + ISBN to borrow a book |
| Return Book | Enter Transaction ID to return a book and see any fine |
| Active Loans | Lists all currently borrowed books with overdue status |

---

### 4. Reports

| Option | Description |
|--------|-------------|
| Transaction History | Full ledger of all checkouts and returns |
| Fine Report | Lists all fines charged with total amount |

---

## 🗄️ Data Storage

Data is automatically saved when you exit and reloaded on next launch.

### books.csv
```
isbn,title,author,totalCopies,availableCopies
9780061965784,The Alchemist,Paulo Coelho,3,2
```

### borrowers.csv
```
id,name,contact
B001,Alice Johnson,alice@email.com
```

### transactions.csv
```
txnId,borrowerId,isbn,checkoutDate,dueDate,returnDate,fine
1001,B001,9780061965784,2025-05-01,2025-05-15,2025-05-18,6.00
```

---

## 💰 Fine Calculation

| Parameter | Value |
|-----------|-------|
| Loan Period | 14 days |
| Fine Rate | ₹2.00 per overdue day |
| Fine Trigger | Return date > Due date |

**Formula:**
```
Overdue Days = Return Date − Due Date
Fine = Overdue Days × ₹2.00
```

**Example:**
- Due date: `2025-05-15`
- Return date: `2025-05-18`
- Overdue: 3 days → Fine = **₹6.00**

No fine is charged if the book is returned on or before the due date.

---

## 🔄 Sample Workflow

```
1. Add a book
   ISBN: 9780061965784 | Title: The Alchemist | Author: Paulo Coelho | Copies: 3

2. Register a borrower
   ID: B001 | Name: Alice Johnson | Contact: alice@email.com

3. Checkout
   Borrower ID: B001 | ISBN: 9780061965784
   → TXN ID: 1001 | Due: 2025-05-28

4. Return
   Transaction ID: 1001
   → Returned on time: No fine
   → Returned late (e.g. 3 days): Fine = ₹6.00

5. View Reports
   → Active Loans, Transaction History, Fine Summary
```

---

## 🔧 Configuration

To change the loan period or fine rate, edit these constants at the top of the `Library` class in `library_management.cpp`:

```cpp
static constexpr double FINE_PER_DAY     = 2.0;   // Fine in ₹ per overdue day
static constexpr int    LOAN_PERIOD_DAYS = 14;     // Loan duration in days
```

---

## 📌 Notes

- A borrower cannot check out the same book twice simultaneously.
- CSV files are created automatically on first run — no setup needed.
- All data persists between sessions via the CSV files.
- The system supports multiple copies of the same book.
