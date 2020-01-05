#include <QProcess>

#include "./application.h"
#include "./nvim_ui_widget.h"
#include "./nvim_ui_calc.h"

Application::Application(int argc, char* argv[]): QApplication(argc, argv) {
    setAttribute(Qt::AA_MacDontSwapCtrlAndMeta, true);

    std::unique_ptr<QProcess> proc(new QProcess);

    proc->setProgram("nvim");
    QStringList args({"--embed"});
    bool args_for_nvim = false;
    for (int i = 1 ; i < argc ; i += 1) {
        if (strcmp(argv[i], "--") == 0) {
            args_for_nvim = true;
            continue;
        }
        if (args_for_nvim)
            args << argv[i];
    }
    proc->setArguments(args);
    proc->start();

    connect(proc.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            proc.get(), &QIODevice::aboutToClose);

    nvim_controller_.reset(new NvimController(std::move(proc)));
    nvim_controller_->ui_widget()->show();
}
