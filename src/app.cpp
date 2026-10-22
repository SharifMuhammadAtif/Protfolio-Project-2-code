#include "app.h"
#include "config.h"
#include "projector.h"
#include "storage.h"
#include "ui.h"

#include <iomanip>
#include <iostream>
#include <algorithm>
#include <map>

namespace {
Transaction seed(int id, const char* date, TxType type, const char* category,
                 double amount, const char* note) {
    Transaction tx;
    tx.id = id;
    tx.date = date;
    tx.type = type;
    tx.category = category;
    tx.amount = amount;
    tx.note = note;
    return tx;
}
}  // namespace

int App::run() {
    loadOrSeed();

    bool running = true;
    while (running) {
        UI::clearScreen();
        const int choice = menu();
        UI::clearScreen();
        UI::banner();

        switch (choice) {
            case 1: addTransaction(); break;
            case 2: viewTransactions(); break;
            case 3: searchTransactions(); break;
            case 4: editOrDelete(); break;
            case 5: categoryReport(); break;
            case 6: projectionModel(); break;
            case 7: saveLedger(); break;
            case 8: loadLedger(); break;
            case 9: help(); break;
            case 0:
                UI::section("Exit", "closing snapshot");
                UI::notice("Saving a closing snapshot, then leaving cleanly.");
                if (Storage::save(FLOWDESK_DATA_FILE, ledger_.all())) {
                    UI::ok(std::string("Saved ") + std::to_string(ledger_.size()) +
                           " records to " + FLOWDESK_DATA_FILE);
                }
                std::cout << "  Closing balance " << C_BOLD << UI::money(ledger_.balance())
                          << C_RESET << '\n';
                UI::rule();
                UI::ok("Goodbye. FlowDesk finished with return 0.");
                running = false;
                break;
            default:
                UI::warn("That option is not on the menu. Pick 0-9.");
                UI::pause();
                break;
        }
    }
    return 0;
}

void App::loadOrSeed() {
    std::vector<Transaction> loaded;
    if (Storage::load(FLOWDESK_DATA_FILE, loaded) && !loaded.empty()) {
        ledger_.replace(std::move(loaded));
        return;
    }

    /* Sample office month so the first demo is not an empty book. */
    ledger_.add(seed(1, "2026-07-01", TxType::Income,  "Consulting", 4200.00, "July retainers"));
    ledger_.add(seed(2, "2026-07-04", TxType::Expense, "Software",     49.00, "Design tools"));
    ledger_.add(seed(3, "2026-07-12", TxType::Expense, "Office",      186.40, "Paper and toner"));
    ledger_.add(seed(4, "2026-08-01", TxType::Income,  "Consulting", 3900.00, "August retainers"));
    ledger_.add(seed(5, "2026-08-08", TxType::Expense, "Travel",      214.75, "Client visit"));
    ledger_.add(seed(6, "2026-08-20", TxType::Expense, "Software",     49.00, "Design tools"));
    ledger_.add(seed(7, "2026-09-01", TxType::Income,  "Consulting", 4100.00, "September retainers"));
    ledger_.add(seed(8, "2026-09-09", TxType::Expense, "Office",       72.30, "Supplies"));
    ledger_.add(seed(9, "2026-09-18", TxType::Expense, "Marketing",   160.00, "Listing refresh"));
}

int App::menu() const {
    UI::banner();

    /* summary strip: income / expense / balance at a glance */
    std::cout << "  " << C_MUTED << std::left << std::setw(22) << "Total income"
              << std::setw(22) << "Total expense" << "Balance" << C_RESET << std::right << '\n'
              << "  " << C_GOOD << std::left << std::setw(22) << UI::money(ledger_.totalIncome())
              << C_BAD << std::setw(22) << UI::money(ledger_.totalExpense())
              << C_BOLD << UI::money(ledger_.balance()) << C_RESET << std::right << '\n'
              << C_MUTED << "  " << ledger_.size() << " records on file   |   today " << UI::today()
              << C_RESET << '\n';
    UI::thinRule();

    auto group = [](const char* name) {
        std::cout << C_ACCENT << "\n  " << name << C_RESET << '\n';
    };
    auto item = [](int key, const char* label) {
        std::cout << "    " << C_BOLD << "[" << key << "]" << C_RESET << "  " << label << '\n';
    };

    group("RECORDS");
    item(1, "Add a transaction");
    item(2, "View all transactions");
    item(3, "Search / filter");
    item(4, "Edit or delete a transaction");
    group("REPORTS");
    item(5, "Category report");
    item(6, "Cash-flow projection model");
    group("FILE");
    item(7, "Save ledger to file");
    item(8, "Load ledger from file");
    group("OTHER");
    item(9, "Help");
    item(0, "Exit");
    std::cout << '\n';
    UI::thinRule();
    return UI::readInt("Choose an option (0-9): ", 0, 9);
}

void App::printTable(const std::vector<Transaction>& rows) const {
    std::cout << C_BOLD << "  " << std::left
              << std::setw(5) << "#"
              << std::setw(13) << "Date"
              << std::setw(10) << "Type"
              << std::setw(14) << "Category"
              << std::right << std::setw(14) << "Amount"
              << "   " << "Note" << C_RESET << '\n';
    UI::thinRule();

    if (rows.empty()) {
        UI::notice("No rows to show.");
        UI::rule();
        return;
    }

    double in = 0.0;
    double out = 0.0;
    for (const Transaction& tx : rows) {
        const char* tone = (tx.type == TxType::Income) ? C_GOOD : C_BAD;
        (tx.type == TxType::Income ? in : out) += tx.amount;
        std::cout << "  " << std::left
                  << std::setw(5) << tx.id
                  << std::setw(13) << tx.date
                  << tone << std::setw(10) << tx.typeLabel() << C_RESET
                  << std::setw(14) << tx.category.substr(0, 13)
                  << std::right << tone << std::setw(14) << UI::money(tx.signedAmount()) << C_RESET
                  << "   " << C_MUTED << tx.note.substr(0, 22) << C_RESET << '\n';
    }
    UI::rule();
    std::cout << "  " << rows.size() << (rows.size() == 1 ? " row" : " rows") << " shown\n"
              << "  Income  " << C_GOOD << std::setw(16) << UI::money(in) << C_RESET << '\n'
              << "  Expense " << C_BAD << std::setw(16) << UI::money(-out) << C_RESET << '\n'
              << "  Net     " << C_BOLD << std::setw(16) << UI::money(in - out) << C_RESET << '\n';
}

void App::addTransaction() {
    UI::section("Add a transaction", "income or expense");
    UI::notice("Leave the date blank to use today.");

    Transaction tx;
    tx.date = UI::readLineOrDefault("Date (YYYY-MM-DD): ", UI::today());
    const int kind = UI::readInt("Type  [1 Income / 2 Expense]: ", 1, 2);
    tx.type = (kind == 1) ? TxType::Income : TxType::Expense;
    tx.category = UI::readLineOrDefault("Category: ", "General");
    tx.amount = UI::readDouble("Amount: ", 0.01, 100000000.0);
    tx.note = UI::readLine("Note (optional): ");

    if (ledger_.add(tx)) {
        UI::ok("Recorded as #" + std::to_string(ledger_.nextId() - 1) + ".");
    } else {
        UI::warn("The ledger is full (see FLOWDESK_MAX_RECORDS).");
    }
    UI::pause();
}

void App::viewTransactions() const {
    UI::section("All transactions", "full ledger");
    printTable(ledger_.all());
    UI::pause();
}

void App::searchTransactions() const {
    UI::section("Search / filter");
    UI::notice("Match against date, type, category, or note.");
    const std::string query = UI::readLine("Search text: ");
    printTable(ledger_.search(query));
    UI::pause();
}

void App::editOrDelete() {
    UI::section("Edit or delete");
    printTable(ledger_.all());
    if (ledger_.size() == 0) {
        UI::pause();
        return;
    }

    const int id = UI::readInt("Transaction ID: ", 1, 100000);
    Transaction* found = ledger_.find(id);
    if (!found) {
        UI::warn("No transaction uses that ID.");
        UI::pause();
        return;
    }

    const int action = UI::readInt("1 Edit   2 Delete   0 Cancel: ", 0, 2);
    if (action == 0) {
        UI::notice("No change made.");
        UI::pause();
        return;
    }
    if (action == 2) {
        if (UI::confirm("Delete this row")) {
            ledger_.remove(id);
            UI::ok("Deleted.");
        } else {
            UI::notice("Kept.");
        }
        UI::pause();
        return;
    }

    Transaction edited = *found;
    edited.date = UI::readLineOrDefault("Date [" + found->date + "]: ", found->date);
    edited.category = UI::readLineOrDefault("Category [" + found->category + "]: ", found->category);
    const std::string amountText = UI::readLine("Amount [" + UI::money(found->amount) + "]: ");
    if (!amountText.empty()) {
        try {
            edited.amount = std::stod(amountText);
        } catch (...) {
            UI::warn("Amount left unchanged.");
        }
    }
    edited.note = UI::readLineOrDefault("Note [" + found->note + "]: ", found->note);
    ledger_.update(id, edited);
    UI::ok("Updated.");
    UI::pause();
}

void App::categoryReport() const {
    UI::section("Category report", "share of each side");

    std::map<std::string, double> income;
    std::map<std::string, double> expense;
    for (const Transaction& tx : ledger_.all()) {
        if (tx.type == TxType::Income) {
            income[tx.category] += tx.amount;
        } else {
            expense[tx.category] += tx.amount;
        }
    }

    /* one block per side, largest category first, with a share bar */
    auto block = [](const char* title, const char* tone,
                    const std::map<std::string, double>& totals) {
        std::cout << tone << C_BOLD << "\n  " << title << C_RESET << '\n';
        if (totals.empty()) {
            UI::notice("None yet.");
            return;
        }
        std::vector<std::pair<std::string, double>> rows(totals.begin(), totals.end());
        std::sort(rows.begin(), rows.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        double sum = 0.0;
        for (const auto& row : rows) {
            sum += row.second;
        }
        for (const auto& row : rows) {
            const double share = (sum > 0.0) ? row.second / sum * 100.0 : 0.0;
            const int bars = static_cast<int>(share / 5.0 + 0.5);  /* 20 bars = 100% */
            std::cout << "    " << std::left << std::setw(14) << row.first.substr(0, 13)
                      << std::right << std::setw(14) << UI::money(row.second)
                      << std::setw(7) << std::fixed << std::setprecision(1) << share << "%  "
                      << tone << std::string(static_cast<std::size_t>(bars), '#') << C_RESET
                      << '\n';
        }
        std::cout << C_MUTED << "    " << std::left << std::setw(14) << "Total"
                  << std::right << std::setw(14) << UI::money(sum) << C_RESET << '\n';
    };

    block("INCOME", C_GOOD, income);
    block("EXPENSE", C_BAD, expense);
    std::cout << '\n';
    UI::rule();
    std::cout << "  Balance " << C_BOLD << UI::money(ledger_.balance()) << C_RESET << '\n';
    UI::pause();
}

void App::projectionModel() const {
    UI::section("Cash-flow projection model", "three scenarios");
    UI::notice("Uses average monthly income and expense from the ledger.");
    UI::notice("Optimistic = expenses x " FLOWDESK_STR(FLOWDESK_OPTIMISTIC)
               "   Cautious = expenses x " FLOWDESK_STR(FLOWDESK_CAUTIOUS));

    double income = ledger_.averageMonthlyIncome();
    double expense = ledger_.averageMonthlyExpense();
    if (ledger_.size() == 0) {
        UI::warn("The book is empty, so enter assumed monthly figures.");
        income = UI::readDouble("Assumed monthly income: ", 0.0, 100000000.0);
        expense = UI::readDouble("Assumed monthly expense: ", 0.0, 100000000.0);
    }

    std::cout << '\n' << std::left
              << "  " << std::setw(22) << "Starting balance" << std::right << std::setw(16)
              << UI::money(ledger_.balance()) << '\n' << std::left
              << "  " << std::setw(22) << "Avg monthly income" << std::right << C_GOOD
              << std::setw(16) << UI::money(income) << C_RESET << '\n' << std::left
              << "  " << std::setw(22) << "Avg monthly expense" << std::right << C_BAD
              << std::setw(16) << UI::money(expense) << C_RESET << '\n' << std::left
              << "  " << std::setw(22) << "Avg monthly net" << std::right << C_BOLD
              << std::setw(16) << UI::money(income - expense) << C_RESET << "\n\n";

    const int months = UI::readInt("How many months ahead (1-36)? ", 1, 36);
    const bool interest = UI::confirm("Apply " FLOWDESK_STR(FLOWDESK_ANNUAL_RATE)
                                     " annual interest on a positive balance");

    const Projector model(ledger_.balance(), income, expense);
    const std::vector<MonthForecast> rows = model.run(months, interest);

    UI::rule();
    std::cout << C_BOLD << "  " << std::left << std::setw(8) << "Month" << std::right
              << std::setw(18) << "Optimistic"
              << std::setw(18) << "Base"
              << std::setw(18) << "Cautious" << C_RESET << '\n';
    UI::thinRule();
    for (const MonthForecast& row : rows) {
        std::cout << "  " << std::left << std::setw(8) << ("M" + std::to_string(row.month))
                  << std::right
                  << C_GOOD << std::setw(18) << UI::money(row.optimistic) << C_RESET
                  << C_BOLD << std::setw(18) << UI::money(row.base) << C_RESET
                  << C_WARN << std::setw(18) << UI::money(row.cautious) << C_RESET << '\n';
    }
    UI::rule();
    if (!rows.empty()) {
        const MonthForecast& last = rows.back();
        std::cout << "  After " << months << (months == 1 ? " month" : " months")
                  << " (interest " << (interest ? "on" : "off") << "):\n"
                  << "    Base case growth  " << C_BOLD
                  << UI::money(last.base - ledger_.balance()) << C_RESET << '\n'
                  << "    Scenario range    " << C_WARN << UI::money(last.cautious) << C_RESET
                  << "  to  " << C_GOOD << UI::money(last.optimistic) << C_RESET << '\n';
    }
    UI::notice("This is a planning model, not a promise.");
    UI::pause();
}

void App::saveLedger() const {
    UI::section("Save ledger");
    if (Storage::save(FLOWDESK_DATA_FILE, ledger_.all())) {
        UI::ok(std::string("Wrote ") + FLOWDESK_DATA_FILE);
    } else {
        UI::warn("Could not write the file. Check the data folder.");
    }
    UI::pause();
}

void App::loadLedger() {
    UI::section("Load ledger");
    std::vector<Transaction> loaded;
    if (!Storage::load(FLOWDESK_DATA_FILE, loaded)) {
        UI::warn("Could not read " FLOWDESK_DATA_FILE);
        UI::pause();
        return;
    }
    ledger_.replace(std::move(loaded));
    UI::ok("Loaded " + std::to_string(ledger_.size()) + " records.");
    UI::pause();
}

void App::help() const {
    UI::section("Help", "about FlowDesk");
    auto line = [](const char* label, const std::string& value) {
        std::cout << "  " << C_MUTED << std::left << std::setw(14) << label << C_RESET
                  << std::right << value << '\n';
    };
    std::cout << "\n  FlowDesk is a small office workstation.\n"
              << "  Record income and expenses, review categories, then project\n"
              << "  cash flow a few months ahead under three scenarios.\n\n";
    line("Data file", FLOWDESK_DATA_FILE);
    line("Build label", FLOWDESK_BUILD_LABEL);
    line("Currency", std::string(FLOWDESK_CURRENCY) + "(set in config.h)");
    line("Language", "C++17   |   Compiler: Clang or GCC");
    line("AI tools", "Cursor (code) and ChatGPT (projection review)");
    std::cout << "\n  Exhaust every menu item, then choose 0. The program\n"
              << "  writes a closing snapshot and returns 0.\n";
    UI::pause();
}
