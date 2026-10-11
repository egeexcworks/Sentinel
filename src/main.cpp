#include "monitoring/SystemMonitor.h"
#include "processes/ProcessMonitor.h"

#include <QApplication>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

#include <algorithm>
#include <string>

namespace
{
    constexpr double BytesPerMiB = 1024.0 * 1024.0;

    QString formatMemory(std::uint64_t bytes)
    {
        const double mib = static_cast<double>(bytes) / BytesPerMiB;

        if (mib >= 1024.0)
        {
            return QString::number(mib / 1024.0, 'f', 2) + " GiB";
        }

        return QString::number(mib, 'f', 1) + " MiB";
    }

    QLabel* createLabel(
        const QString& text,
        int fontSize,
        const QString& color,
        bool bold = false)
    {
        auto* label = new QLabel(text);

        QFont font;
        font.setPointSize(fontSize);
        font.setBold(bold);

        label->setFont(font);
        label->setStyleSheet(
            QString("color: %1;").arg(color));

        return label;
    }

    QFrame* createMetricCard(
        const QString& title,
        QLabel*& valueLabel,
        QProgressBar*& progressBar,
        const QString& accent)
    {
        auto* card = new QFrame;
        card->setObjectName("metricCard");
        card->setMinimumHeight(155);

        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(22, 20, 22, 20);
        layout->setSpacing(12);

        auto* heading = createLabel(title, 11, "#9CA3AF", true);

        valueLabel = createLabel("--", 27, "#F9FAFB", true);

        progressBar = new QProgressBar;
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(false);
        progressBar->setFixedHeight(7);

        progressBar->setStyleSheet(
            QString(
                "QProgressBar {"
                " background: #272B36;"
                " border: none;"
                " border-radius: 3px;"
                "}"
                "QProgressBar::chunk {"
                " background: %1;"
                " border-radius: 3px;"
                "}"
            ).arg(accent));

        layout->addWidget(heading);
        layout->addWidget(valueLabel);
        layout->addStretch();
        layout->addWidget(progressBar);

        return card;
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("Sentinel");
    QApplication::setOrganizationName("Eclips");

    SystemMonitor systemMonitor;
    ProcessMonitor processMonitor;

    std::string error;

    if (!systemMonitor.initialize(error))
    {
        QMessageBox::critical(
            nullptr,
            "Sentinel - Initialization Failed",
            QString::fromStdString(error));

        return 1;
    }

    QMainWindow window;
    window.setWindowTitle("Sentinel | System Monitor");
    window.resize(1120, 760);
    window.setMinimumSize(850, 600);

    auto* central = new QWidget;
    central->setObjectName("central");
    window.setCentralWidget(central);

    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar
    auto* sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(215);

    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(20, 26, 20, 20);
    sidebarLayout->setSpacing(14);

    auto* brand = createLabel("SENTINEL", 21, "#F9FAFB", true);
    auto* subtitle = createLabel(
        "SYSTEM INTELLIGENCE", 8, "#7C8496", true);

    auto* brandArea = new QVBoxLayout;
    brandArea->setSpacing(5);
    brandArea->addWidget(brand);
    brandArea->addWidget(subtitle);

    sidebarLayout->addLayout(brandArea);
    sidebarLayout->addSpacing(35);

    auto* navHeading = createLabel(
        "MONITORING", 9, "#7C8496", true);

    auto* overviewButton = new QPushButton("  Overview");
    overviewButton->setObjectName("activeNav");
    overviewButton->setCursor(Qt::PointingHandCursor);
    overviewButton->setMinimumHeight(42);

    auto* processesButton = new QPushButton("  Processes");
    processesButton->setObjectName("navButton");
    processesButton->setCursor(Qt::PointingHandCursor);
    processesButton->setMinimumHeight(42);

    sidebarLayout->addWidget(navHeading);
    sidebarLayout->addWidget(overviewButton);
    sidebarLayout->addWidget(processesButton);
    sidebarLayout->addStretch();

    auto* version = createLabel(
        "SENTINEL 0.2.0\nWindows Edition",
        9,
        "#7C8496");

    sidebarLayout->addWidget(version);

    root->addWidget(sidebar);

    // Main content
    auto* content = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);

    contentLayout->setContentsMargins(30, 26, 30, 24);
    contentLayout->setSpacing(22);

    auto* topBar = new QHBoxLayout;

    auto* headingLayout = new QVBoxLayout;
    headingLayout->setSpacing(5);

    auto* pageTitle = createLabel(
        "System Overview", 24, "#F9FAFB", true);

    auto* pageDescription = createLabel(
        "Live performance and resource monitoring",
        10,
        "#9CA3AF");

    headingLayout->addWidget(pageTitle);
    headingLayout->addWidget(pageDescription);

    auto* status = createLabel(
        "●  MONITORING", 9, "#4ADE80", true);

    topBar->addLayout(headingLayout);
    topBar->addStretch();
    topBar->addWidget(status, 0, Qt::AlignVCenter);

    contentLayout->addLayout(topBar);

    // CPU and RAM cards
    QLabel* cpuValue = nullptr;
    QLabel* ramValue = nullptr;

    QProgressBar* cpuProgress = nullptr;
    QProgressBar* ramProgress = nullptr;

    auto* cards = new QHBoxLayout;
    cards->setSpacing(18);

    cards->addWidget(createMetricCard(
        "CPU UTILIZATION",
        cpuValue,
        cpuProgress,
        "#818CF8"));

    cards->addWidget(createMetricCard(
        "PHYSICAL MEMORY",
        ramValue,
        ramProgress,
        "#38BDF8"));

    contentLayout->addLayout(cards);

    // Process section heading
    auto* processHeading = new QHBoxLayout;

    auto* processTitle = createLabel(
        "Running Processes", 16, "#F9FAFB", true);

    auto* processSubtitle = createLabel(
        "Sorted by memory usage", 9, "#9CA3AF");

    processHeading->addWidget(processTitle);
    processHeading->addStretch();
    processHeading->addWidget(processSubtitle);

    contentLayout->addLayout(processHeading);

    // Process table
    auto* table = new QTableWidget;
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(
        {"PROCESS", "PID", "MEMORY"});

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setAlternatingRowColors(false);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(39);

    table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);

    table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);

    table->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);

    table->setMinimumHeight(250);

    contentLayout->addWidget(table, 1);

    auto* footer = new QHBoxLayout;

    auto* updateLabel = createLabel(
        "Waiting for first update...",
        9,
        "#7C8496");

    auto* refreshButton = new QPushButton("Refresh now");
    refreshButton->setObjectName("refreshButton");
    refreshButton->setCursor(Qt::PointingHandCursor);

    footer->addWidget(updateLabel);
    footer->addStretch();
    footer->addWidget(refreshButton);

    contentLayout->addLayout(footer);
    root->addWidget(content, 1);

    // Dark theme
    window.setStyleSheet(R"(
        QWidget#central {
            background: #11131A;
        }

        QFrame#sidebar {
            background: #171922;
            border-right: 1px solid #292D39;
        }

        QFrame#metricCard {
            background: #1A1D27;
            border: 1px solid #2A2E3B;
            border-radius: 12px;
        }

        QPushButton {
            color: #A5ADBD;
            background: transparent;
            border: none;
            border-radius: 7px;
            text-align: left;
            padding: 10px;
        }

        QPushButton#activeNav {
            color: #FFFFFF;
            background: #30334A;
            font-weight: bold;
        }

        QPushButton#navButton:hover {
            color: #FFFFFF;
            background: #242733;
        }

        QPushButton#refreshButton {
            color: #FFFFFF;
            background: #4F46E5;
            padding: 9px 16px;
            border-radius: 7px;
        }

        QPushButton#refreshButton:hover {
            background: #6366F1;
        }

        QTableWidget {
            background: #1A1D27;
            alternate-background-color: #1E212C;
            color: #E5E7EB;
            border: 1px solid #2A2E3B;
            border-radius: 9px;
            selection-background-color: #30334A;
            selection-color: #FFFFFF;
        }

        QHeaderView::section {
            background: #1A1D27;
            color: #8992A5;
            border: none;
            border-bottom: 1px solid #2A2E3B;
            padding: 12px;
            font-size: 10px;
            font-weight: bold;
        }

        QTableWidget::item {
            padding-left: 10px;
            border: none;
        }

        QScrollBar:vertical {
            background: #1A1D27;
            width: 8px;
            margin: 0;
        }

        QScrollBar::handle:vertical {
            background: #414657;
            min-height: 25px;
            border-radius: 4px;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");

    // Update metrics and processes.
    auto refresh = [&]()
    {
        SystemMetrics metrics;
        std::string sampleError;

        if (systemMonitor.sample(metrics, sampleError))
        {
            const double cpu = std::clamp(
                metrics.cpuUsagePercent, 0.0, 100.0);

            cpuValue->setText(
                QString::number(cpu, 'f', 1) + "%");

            cpuProgress->setValue(
                static_cast<int>(cpu));

            const double used = static_cast<double>(
                metrics.memoryUsedBytes);

            const double total = static_cast<double>(
                metrics.memoryTotalBytes);

            const double ramPercent =
                total > 0.0 ? (used / total) * 100.0 : 0.0;

            ramValue->setText(
                formatMemory(metrics.memoryUsedBytes)
                + " / "
                + formatMemory(metrics.memoryTotalBytes));

            ramProgress->setValue(
                static_cast<int>(
                    std::clamp(ramPercent, 0.0, 100.0)));
        }
        else
        {
            status->setText("●  MONITORING ERROR");
            status->setStyleSheet("color: #F87171;");
        }

        const auto processes = processMonitor.getProcesses();

        table->setUpdatesEnabled(false);
        table->setRowCount(0);

        const int count = static_cast<int>(
            std::min<std::size_t>(processes.size(), 20));

        table->setRowCount(count);

        for (int i = 0; i < count; ++i)
        {
            const auto& process =
                processes[static_cast<std::size_t>(i)];

            auto* nameItem = new QTableWidgetItem(
                QString::fromStdString(process.name));

            auto* pidItem = new QTableWidgetItem(
                QString::number(process.processId));

            auto* memoryItem = new QTableWidgetItem(
                formatMemory(process.memoryBytes));

            pidItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            memoryItem->setTextAlignment(
                Qt::AlignRight | Qt::AlignVCenter);

            table->setItem(i, 0, nameItem);
            table->setItem(i, 1, pidItem);
            table->setItem(i, 2, memoryItem);
        }

        table->setUpdatesEnabled(true);

        updateLabel->setText(
            "Last updated: "
            + QDateTime::currentDateTime().toString("hh:mm:ss")
            + "  |  Refresh interval: 2 seconds");
    };

    auto* timer = new QTimer(&window);
    timer->setInterval(2000);

    QObject::connect(
        timer, &QTimer::timeout,
        &window, refresh);

    QObject::connect(
        refreshButton, &QPushButton::clicked,
        &window, refresh);

    QObject::connect(
        overviewButton, &QPushButton::clicked,
        &window, [&]()
        {
            pageTitle->setText("System Overview");
            table->show();
            processTitle->show();
            processSubtitle->show();
        });

    QObject::connect(
        processesButton, &QPushButton::clicked,
        &window, [&]()
        {
            pageTitle->setText("Running Processes");
            table->show();
            processTitle->show();
            processSubtitle->show();
        });

    refresh();
    timer->start();

    window.show();

    return app.exec();
}