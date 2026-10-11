
#include "monitoring/SystemMonitor.h"
#include "processes/ProcessMonitor.h"

#include <QApplication>
#include <QDateTime>
#include <QFont>
#include <QColor>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QAbstractItemView>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace SentinelUI
{
constexpr double MiB = 1024.0 * 1024.0;

const QString TextPrimary = "#202124";
const QString TextSecondary = "#666B73";
const QString Border = "#DADCE0";
const QString Accent = "#176B87";

QString memoryText(std::uint64_t bytes)
{
    const double value = static_cast<double>(bytes);

    if (value >= MiB * 1024.0)
        return QString::number(value / (MiB * 1024.0), 'f', 2)
            + " GiB";

    return QString::number(value / MiB, 'f', 1) + " MiB";
}

QLabel* label(
    const QString& text,
    int size = 10,
    const QString& color = TextPrimary,
    bool bold = false)
{
    auto* result = new QLabel(text);

    QFont font = result->font();
    font.setPointSize(size);
    font.setBold(bold);

    result->setFont(font);
    result->setStyleSheet(QString("color: %1;").arg(color));

    return result;
}

QFrame* separator()
{
    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFixedHeight(1);
    line->setStyleSheet(
        QString("background: %1; border: none;").arg(Border));

    return line;
}

class NumericItem final : public QTableWidgetItem
{
public:
    NumericItem(const QString& text, std::uint64_t value)
        : QTableWidgetItem(text), value_(value)
    {
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        const auto* numeric =
            dynamic_cast<const NumericItem*>(&other);

        if (numeric)
            return value_ < numeric->value_;

        return QTableWidgetItem::operator<(other);
    }

private:
    std::uint64_t value_;
};

class CpuItem final : public QTableWidgetItem
{
public:
    explicit CpuItem(double value)
        : QTableWidgetItem(QString::number(value, 'f', 1) + "%"),
          value_(value)
    {
        setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }

    bool operator<(const QTableWidgetItem& other) const override
    {
        const auto* cpu = dynamic_cast<const CpuItem*>(&other);

        if (cpu)
            return value_ < cpu->value_;

        return QTableWidgetItem::operator<(other);
    }

private:
    double value_;
};

class HistoryChart final : public QWidget
{
public:
    HistoryChart(
        const QString& title,
        const QString& unit,
        const QColor& color,
        QWidget* parent = nullptr)
        : QWidget(parent),
          title_(title),
          unit_(unit),
          color_(color)
    {
        setMinimumHeight(160);
    }

    void setHistory(const std::vector<double>& values)
    {
        values_ = values;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QRect area = rect().adjusted(12, 12, -12, -12);

        QFont titleFont = painter.font();
        titleFont.setPointSize(10);
        titleFont.setBold(true);

        painter.setFont(titleFont);
        painter.setPen(QColor(TextPrimary));
        painter.drawText(area.left(), area.top() + 12, title_);

        const double latest =
            values_.empty() ? 0.0 : values_.back();

        QFont smallFont = painter.font();
        smallFont.setPointSize(9);
        smallFont.setBold(false);

        painter.setFont(smallFont);
        painter.setPen(QColor(TextSecondary));

        painter.drawText(
            area.right() - 90,
            area.top() + 12,
            90,
            15,
            Qt::AlignRight,
            QString::number(latest, 'f', 1) + unit_);

        const QRectF graph(
            area.left(),
            area.top() + 30,
            area.width(),
            area.height() - 35);

        painter.setPen(QPen(QColor("#E8EAED"), 1));

        for (int i = 0; i < 4; ++i)
        {
            const qreal y =
                graph.top() + graph.height() * i / 3.0;

            painter.drawLine(
                QPointF(graph.left(), y),
                QPointF(graph.right(), y));
        }

        if (values_.size() < 2)
        {
            painter.drawText(
                graph,
                Qt::AlignCenter,
                "Collecting performance history...");
            return;
        }

        QPainterPath path;

        for (std::size_t i = 0; i < values_.size(); ++i)
        {
            const double value =
                std::clamp(values_[i], 0.0, 100.0);

            const qreal x =
                graph.left() + graph.width()
                * static_cast<qreal>(i)
                / static_cast<qreal>(values_.size() - 1);

            const qreal y =
                graph.bottom() - graph.height() * value / 100.0;

            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }

        painter.setPen(QPen(color_, 2));
        painter.drawPath(path);
    }

private:
    QString title_;
    QString unit_;
    QColor color_;
    std::vector<double> values_;
};

QFrame* metricPanel(
    const QString& heading,
    QLabel*& value,
    QLabel*& detail,
    QProgressBar*& bar,
    const QString& color)
{
    auto* panel = new QFrame;
    panel->setObjectName("metricPanel");

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(7);

    layout->addWidget(label(heading.toUpper(), 9, TextSecondary, true));

    value = label("--", 22, TextPrimary, true);
    detail = label("Waiting for data", 9, TextSecondary);

    bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(false);
    bar->setFixedHeight(5);
    bar->setStyleSheet(QString(
        "QProgressBar { background: #E9ECEF; border: none; }"
        "QProgressBar::chunk { background: %1; }").arg(color));

    layout->addWidget(value);
    layout->addWidget(detail);
    layout->addStretch();
    layout->addWidget(bar);

    return panel;
}

QPushButton* navButton(const QString& text)
{
    auto* button = new QPushButton(text);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(36);

    button->setStyleSheet(R"(
        QPushButton {
            color: #454A52;
            background: transparent;
            border: none;
            border-radius: 4px;
            text-align: left;
            padding: 0 12px;
            font-size: 10pt;
        }
        QPushButton:hover { background: #E9EDF1; }
        QPushButton:checked {
            color: #135E78;
            background: #E2F0F5;
            font-weight: 600;
            border-left: 3px solid #176B87;
            padding-left: 9px;
        }
    )");

    return button;
}

void configureProcessTable(QTableWidget* table)
{
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(
        {"Process", "PID", "CPU", "Memory"});

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSortingEnabled(true);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(32);
    table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);
    table->setShowGrid(false);
}

void fillProcessTable(
    QTableWidget* table,
    const std::vector<ProcessInfo>& processes,
    const QString& filter,
    int maximumRows)
{
    const bool wasSortingEnabled = table->isSortingEnabled();

    table->setSortingEnabled(false);
    table->setUpdatesEnabled(false);
    table->setRowCount(0);

    int row = 0;

    for (const auto& process : processes)
    {
        const QString name = QString::fromStdString(process.name);
        const QString pid = QString::number(process.processId);

        if (!filter.isEmpty() &&
            !name.contains(filter, Qt::CaseInsensitive) &&
            !pid.contains(filter, Qt::CaseInsensitive))
        {
            continue;
        }

        if (row >= maximumRows)
            break;

        table->insertRow(row);

        table->setItem(row, 0, new QTableWidgetItem(name));
        table->setItem(
            row, 1,
            new NumericItem(
                pid, static_cast<std::uint64_t>(process.processId)));
        table->setItem(row, 2, new CpuItem(process.cpuUsagePercent));
        table->setItem(
            row, 3,
            new NumericItem(
                memoryText(process.memoryBytes), process.memoryBytes));

        ++row;
    }

    table->setUpdatesEnabled(true);
    table->setSortingEnabled(wasSortingEnabled);
}
}

int main(int argc, char* argv[])
{
    using namespace SentinelUI;

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
            "Sentinel",
            QString("Could not initialize system monitoring:\n%1")
                .arg(QString::fromStdString(error)));

        return 1;
    }

    QMainWindow window;
    window.setWindowTitle("Sentinel");
    window.resize(1120, 740);
    window.setMinimumSize(900, 600);

    auto* rootWidget = new QWidget;
    rootWidget->setObjectName("rootWidget");
    window.setCentralWidget(rootWidget);

    auto* root = new QHBoxLayout(rootWidget);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar
    auto* sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(205);

    auto* side = new QVBoxLayout(sidebar);
    side->setContentsMargins(14, 18, 14, 14);
    side->setSpacing(5);

    side->addWidget(label("Sentinel", 17, "#1D252C", true));
    side->addWidget(label(
        "WINDOWS SYSTEM UTILITY", 8, TextSecondary, true));
    side->addSpacing(22);
    side->addWidget(label("MONITOR", 8, TextSecondary, true));
    side->addSpacing(5);

    auto* overviewNav = navButton("Overview");
    auto* processesNav = navButton("Processes");
    auto* performanceNav = navButton("Performance");
    auto* diagnosticsNav = navButton("Diagnostics");

    overviewNav->setChecked(true);

    side->addWidget(overviewNav);
    side->addWidget(processesNav);
    side->addWidget(performanceNav);
    side->addWidget(diagnosticsNav);
    side->addStretch();
    side->addWidget(separator());
    side->addSpacing(8);
    side->addWidget(label("Sentinel 0.4.0", 9, TextSecondary));
    side->addWidget(label("Local monitoring", 8, TextSecondary));

    root->addWidget(sidebar);

    // Main content
    auto* mainArea = new QWidget;
    auto* mainLayout = new QVBoxLayout(mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* header = new QWidget;
    header->setObjectName("header");
    header->setFixedHeight(62);

    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 0, 24, 0);

    auto* pageTitle = label("Overview", 16, TextPrimary, true);
    auto* liveStatus = label("● Monitoring", 9, "#247A4B", true);

    headerLayout->addWidget(pageTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(liveStatus);

    mainLayout->addWidget(header);
    mainLayout->addWidget(separator());

    auto* pages = new QStackedWidget;
    mainLayout->addWidget(pages, 1);
    root->addWidget(mainArea, 1);

    // Overview page
    auto* overviewPage = new QWidget;
    auto* overviewLayout = new QVBoxLayout(overviewPage);
    overviewLayout->setContentsMargins(24, 20, 24, 20);
    overviewLayout->setSpacing(16);

    auto* summary = new QHBoxLayout;
    summary->setSpacing(12);

    QLabel* cpuValue = nullptr;
    QLabel* cpuDetail = nullptr;
    QProgressBar* cpuBar = nullptr;

    QLabel* ramValue = nullptr;
    QLabel* ramDetail = nullptr;
    QProgressBar* ramBar = nullptr;

    QLabel* processCountValue = nullptr;
    QLabel* processCountDetail = nullptr;
    QProgressBar* processCountBar = nullptr;

    summary->addWidget(metricPanel(
        "CPU utilization", cpuValue, cpuDetail, cpuBar, "#176B87"));

    summary->addWidget(metricPanel(
        "Physical memory", ramValue, ramDetail, ramBar, "#5379A5"));

    summary->addWidget(metricPanel(
        "Processes", processCountValue, processCountDetail,
        processCountBar, "#75808D"));

    overviewLayout->addLayout(summary);

    auto* topHeader = new QHBoxLayout;
    topHeader->addWidget(label("Top processes", 12, TextPrimary, true));
    topHeader->addStretch();
    topHeader->addWidget(label(
        "Highest memory usage", 9, TextSecondary));

    overviewLayout->addLayout(topHeader);

    auto* topTable = new QTableWidget;
    configureProcessTable(topTable);
    overviewLayout->addWidget(topTable, 1);
    pages->addWidget(overviewPage);

    // Processes page
    auto* processesPage = new QWidget;
    auto* processesLayout = new QVBoxLayout(processesPage);
    processesLayout->setContentsMargins(24, 20, 24, 20);
    processesLayout->setSpacing(12);

    auto* processToolbar = new QHBoxLayout;

    auto* processSearch = new QLineEdit;
    processSearch->setPlaceholderText("Search process name or PID...");
    processSearch->setClearButtonEnabled(true);
    processSearch->setMinimumHeight(34);

    auto* processSummary = label("Loading processes...", 9, TextSecondary);

    auto* endTaskButton = new QPushButton("End Task");
    endTaskButton->setMinimumHeight(34);
    endTaskButton->setCursor(Qt::PointingHandCursor);
    endTaskButton->setStyleSheet(R"(
        QPushButton {
            background: #B42318;
            color: white;
            border: none;
            border-radius: 3px;
            padding: 0 14px;
            font-weight: 600;
        }
        QPushButton:hover { background: #912018; }
        QPushButton:disabled {
            background: #C9CDD3;
            color: #666B73;
        }
    )");

    processToolbar->addWidget(processSearch, 1);
    processToolbar->addWidget(processSummary);
    processToolbar->addWidget(endTaskButton);

    processesLayout->addLayout(processToolbar);

    auto* processTable = new QTableWidget;
    configureProcessTable(processTable);
    processesLayout->addWidget(processTable, 1);
    pages->addWidget(processesPage);

    // Performance page
    auto* performancePage = new QWidget;
    auto* performanceLayout = new QVBoxLayout(performancePage);
    performanceLayout->setContentsMargins(24, 20, 24, 20);
    performanceLayout->setSpacing(14);

    performanceLayout->addWidget(
        label("Performance history", 14, TextPrimary, true));

    performanceLayout->addWidget(
        label("Recent samples collected while Sentinel is running.",
              9, TextSecondary));

    auto* cpuChart = new HistoryChart(
        "CPU utilization", "%", QColor("#176B87"));

    auto* ramChart = new HistoryChart(
        "Physical memory usage", "%", QColor("#5379A5"));

    auto* chartFrame1 = new QFrame;
    chartFrame1->setObjectName("chartPanel");
    auto* chartLayout1 = new QVBoxLayout(chartFrame1);
    chartLayout1->setContentsMargins(8, 6, 8, 6);
    chartLayout1->addWidget(cpuChart);

    auto* chartFrame2 = new QFrame;
    chartFrame2->setObjectName("chartPanel");
    auto* chartLayout2 = new QVBoxLayout(chartFrame2);
    chartLayout2->setContentsMargins(8, 6, 8, 6);
    chartLayout2->addWidget(ramChart);

    performanceLayout->addWidget(chartFrame1, 1);
    performanceLayout->addWidget(chartFrame2, 1);
    pages->addWidget(performancePage);

    // Diagnostics page
    auto* diagnosticsPage = new QWidget;
    auto* diagnosticsLayout = new QVBoxLayout(diagnosticsPage);
    diagnosticsLayout->setContentsMargins(24, 20, 24, 20);
    diagnosticsLayout->setSpacing(12);

    diagnosticsLayout->addWidget(
        label("System diagnostics", 14, TextPrimary, true));

    diagnosticsLayout->addWidget(
        label("Current monitoring status and resource details.",
              9, TextSecondary));

    auto* diagnosticsFrame = new QFrame;
    diagnosticsFrame->setObjectName("diagnosticsPanel");

    auto* diagnosticsGrid = new QGridLayout(diagnosticsFrame);
    diagnosticsGrid->setContentsMargins(18, 16, 18, 16);
    diagnosticsGrid->setHorizontalSpacing(20);
    diagnosticsGrid->setVerticalSpacing(14);

    auto* monitorDiag = label("Initialized", 10, "#247A4B", true);
    auto* cpuDiag = label("Waiting for sample", 10);
    auto* ramDiag = label("Waiting for sample", 10);
    auto* processDiag = label("Waiting for sample", 10);
    auto* lastUpdated = label("Not updated yet", 10);

    diagnosticsGrid->addWidget(
        label("Monitoring engine", 9, TextSecondary), 0, 0);
    diagnosticsGrid->addWidget(monitorDiag, 0, 1);

    diagnosticsGrid->addWidget(
        label("CPU reading", 9, TextSecondary), 1, 0);
    diagnosticsGrid->addWidget(cpuDiag, 1, 1);

    diagnosticsGrid->addWidget(
        label("Memory reading", 9, TextSecondary), 2, 0);
    diagnosticsGrid->addWidget(ramDiag, 2, 1);

    diagnosticsGrid->addWidget(
        label("Process enumeration", 9, TextSecondary), 3, 0);
    diagnosticsGrid->addWidget(processDiag, 3, 1);

    diagnosticsGrid->addWidget(
        label("Last successful update", 9, TextSecondary), 4, 0);
    diagnosticsGrid->addWidget(lastUpdated, 4, 1);

    diagnosticsLayout->addWidget(diagnosticsFrame);
    diagnosticsLayout->addStretch();
    pages->addWidget(diagnosticsPage);

    // Styling
    window.setStyleSheet(R"(
        QWidget#rootWidget { background: #F5F6F8; }
        QFrame#sidebar {
            background: #FFFFFF;
            border-right: 1px solid #DADCE0;
        }
        QWidget#header { background: #FFFFFF; }
        QFrame#metricPanel, QFrame#chartPanel, QFrame#diagnosticsPanel {
            background: #FFFFFF;
            border: 1px solid #DADCE0;
            border-radius: 3px;
        }
        QTableWidget {
            background: #FFFFFF;
            alternate-background-color: #F8F9FA;
            color: #202124;
            border: 1px solid #DADCE0;
            selection-background-color: #DCECF2;
            selection-color: #173F4D;
            outline: none;
        }
        QHeaderView::section {
            background: #F0F2F4;
            color: #50565E;
            border: none;
            border-bottom: 1px solid #DADCE0;
            padding: 8px 10px;
            font-size: 9pt;
            font-weight: 600;
        }
        QTableWidget::item {
            padding-left: 8px;
            border: none;
        }
        QLineEdit {
            background: #FFFFFF;
            border: 1px solid #C9CDD3;
            border-radius: 3px;
            padding: 6px 9px;
            color: #202124;
            selection-background-color: #176B87;
        }
        QLineEdit:focus { border: 1px solid #176B87; }
        QScrollBar:vertical {
            background: #F5F6F8;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #C9CDD3;
            min-height: 24px;
            border-radius: 3px;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0; }
        QStatusBar {
            background: #FFFFFF;
            color: #666B73;
            border-top: 1px solid #DADCE0;
        }
    )");

    // Navigation
    auto showPage = [&](int index,
                        const QString& title,
                        QPushButton* activeButton)
    {
        pages->setCurrentIndex(index);
        pageTitle->setText(title);

        overviewNav->setChecked(activeButton == overviewNav);
        processesNav->setChecked(activeButton == processesNav);
        performanceNav->setChecked(activeButton == performanceNav);
        diagnosticsNav->setChecked(activeButton == diagnosticsNav);
    };

    QObject::connect(overviewNav, &QPushButton::clicked, &window, [&]()
    {
        showPage(0, "Overview", overviewNav);
    });

    QObject::connect(processesNav, &QPushButton::clicked, &window, [&]()
    {
        showPage(1, "Processes", processesNav);
    });

    QObject::connect(performanceNav, &QPushButton::clicked, &window, [&]()
    {
        showPage(2, "Performance", performanceNav);
    });

    QObject::connect(diagnosticsNav, &QPushButton::clicked, &window, [&]()
    {
        showPage(3, "Diagnostics", diagnosticsNav);
    });

    std::vector<ProcessInfo> currentProcesses;
    std::vector<double> cpuHistory;
    std::vector<double> ramHistory;

    auto refreshProcesses = [&]()
    {
        currentProcesses = processMonitor.getProcesses();

        fillProcessTable(topTable, currentProcesses, QString(), 10);

        fillProcessTable(
            processTable,
            currentProcesses,
            processSearch->text().trimmed(),
            10000);

        processSummary->setText(
            QString("%1 processes").arg(currentProcesses.size()));
    };

    QObject::connect(
        processSearch,
        &QLineEdit::textChanged,
        &window,
        [&](const QString& filter)
        {
            fillProcessTable(
                processTable, currentProcesses, filter.trimmed(), 10000);

            int matching = 0;

            for (const auto& process : currentProcesses)
            {
                const QString name = QString::fromStdString(process.name);
                const QString pid = QString::number(process.processId);

                if (filter.trimmed().isEmpty() ||
                    name.contains(filter.trimmed(), Qt::CaseInsensitive) ||
                    pid.contains(filter.trimmed(), Qt::CaseInsensitive))
                {
                    ++matching;
                }
            }

            processSummary->setText(
                QString("%1 matching processes").arg(matching));
        });

    QObject::connect(
        endTaskButton,
        &QPushButton::clicked,
        &window,
        [&]()
        {
            const int row = processTable->currentRow();

            if (row < 0 || processTable->item(row, 1) == nullptr)
            {
                QMessageBox::information(
                    &window,
                    "End Task",
                    "Select a process from the list first.");
                return;
            }

            bool validPid = false;

            const qulonglong rawPid =
                processTable->item(row, 1)->text().toULongLong(&validPid);

            if (!validPid ||
                rawPid > static_cast<qulonglong>(
                    (std::numeric_limits<unsigned long>::max)()))
            {
                QMessageBox::warning(
                    &window,
                    "End Task",
                    "The selected process ID is invalid.");
                return;
            }

            const auto pid = static_cast<unsigned long>(rawPid);
            const QString name = processTable->item(row, 0)->text();

            const auto answer = QMessageBox::question(
                &window,
                "Confirm End Task",
                QString(
                    "Terminate %1 (PID %2)?\n\n"
                    "Unsaved work in this process may be lost.")
                    .arg(name)
                    .arg(pid),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);

            if (answer != QMessageBox::Yes)
                return;

            endTaskButton->setEnabled(false);

            const EndProcessResult result = processMonitor.endProcess(pid);

            endTaskButton->setEnabled(true);

            if (result.success)
            {
                window.statusBar()->showMessage(
                    QString("Termination requested for %1 (PID %2).")
                        .arg(name)
                        .arg(pid),
                    5000);

                // Refresh the list after the process termination request.
                refreshProcesses();
            }
            else
            {
                QMessageBox::warning(
                    &window,
                    "Could Not End Task",
                    QString::fromStdString(result.message));
            }
        });

    // Refresh overall system metrics and process information.
    auto refresh = [&]()
    {
        SystemMetrics metrics;
        std::string sampleError;

        if (!systemMonitor.sample(metrics, sampleError))
        {
            liveStatus->setText("● Monitoring error");
            liveStatus->setStyleSheet("color: #B42318;");
            monitorDiag->setText(QString::fromStdString(sampleError));
            monitorDiag->setStyleSheet("color: #B42318;");
            return;
        }

        liveStatus->setText("● Monitoring");
        liveStatus->setStyleSheet("color: #247A4B;");

        const double cpu = std::clamp(
            metrics.cpuUsagePercent, 0.0, 100.0);

        const double totalBytes =
            static_cast<double>(metrics.memoryTotalBytes);

        const double usedBytes =
            static_cast<double>(metrics.memoryUsedBytes);

        const double ramPercent = totalBytes > 0.0
            ? std::clamp(usedBytes / totalBytes * 100.0, 0.0, 100.0)
            : 0.0;

        cpuValue->setText(QString::number(cpu, 'f', 1) + "%");
        cpuDetail->setText("Total processor utilization");
        cpuBar->setValue(static_cast<int>(cpu));

        ramValue->setText(memoryText(metrics.memoryUsedBytes));
        ramDetail->setText(
            "of " + memoryText(metrics.memoryTotalBytes)
            + " · " + QString::number(ramPercent, 'f', 1) + "%");
        ramBar->setValue(static_cast<int>(ramPercent));

        cpuHistory.push_back(cpu);
        ramHistory.push_back(ramPercent);

        constexpr std::size_t MaxHistory = 60;

        if (cpuHistory.size() > MaxHistory)
        {
            cpuHistory.erase(cpuHistory.begin());
            ramHistory.erase(ramHistory.begin());
        }

        cpuChart->setHistory(cpuHistory);
        ramChart->setHistory(ramHistory);

        refreshProcesses();

        processCountValue->setText(
            QString::number(currentProcesses.size()));
        processCountDetail->setText("Currently enumerated processes");
        processCountBar->setValue(0);

        cpuDiag->setText(QString::number(cpu, 'f', 1) + "%");

        ramDiag->setText(
            memoryText(metrics.memoryUsedBytes)
            + " used of "
            + memoryText(metrics.memoryTotalBytes));

        processDiag->setText(
            QString("%1 processes enumerated")
                .arg(currentProcesses.size()));

        lastUpdated->setText(
            QDateTime::currentDateTime().toString(
                "yyyy-MM-dd hh:mm:ss"));

        window.statusBar()->showMessage(
            "Local monitoring · Last updated "
            + QDateTime::currentDateTime().toString("hh:mm:ss"));
    };

    auto* timer = new QTimer(&window);
    timer->setInterval(2000);

    QObject::connect(timer, &QTimer::timeout, &window, refresh);

    refresh();
    timer->start();

    window.show();

    return app.exec();
}
