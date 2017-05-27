#pragma once

#include <QMainWindow>
#include <QPointer>
#include "DecayScheme.h"
#include "SchemePlayer.h"

namespace Ui {
class NucleiMainWindow;
class PreferencesDialog;
}
class QListWidgetItem;
class QModelIndex;
class QDoubleSpinBox;
class DecayCascadeItemModel;
class DecayCascadeFilterProxyModel;
class QLabel;

class Nuclei : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit Nuclei(QWidget *parent = 0);
    ~Nuclei();

private slots:
    void initialize();

    void loadSelectedDecay(const QModelIndex &index);
    void loadSearchResultCascade(const QModelIndex &index);

    void svgExport();
    void pdfExport();

    void showAll();

    void showPreferences();

    void playerSelectionChanged();
    
protected:
    void closeEvent(QCloseEvent * event);

private:
    void loadDecay(DecayScheme decay);

    Ui::NucleiMainWindow *ui;
    QDialog *preferencesDialog;
    Ui::PreferencesDialog *preferencesDialogUi;

    DecayCascadeItemModel *decaySelectionModel, *searchResultSelectionModel;
    DecayCascadeFilterProxyModel *decayProxyModel, *searchProxyModel;

    QPointer<SchemePlayer> decay_viewer_;
    DecayScheme current_scheme_;

    std::string make_reference_link(std::string ref, int num);
    QString prep_comments(const json& j,
                          const std::set<std::string>& refs);
};
