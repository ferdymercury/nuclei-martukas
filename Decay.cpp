#include "Decay.h"
#include <QDebug>
#include <QLocale>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsDropShadowEffect>
#include <QFontMetrics>
#include <QVector>
#include <cmath>
#include <Akk.h>
#include "EnergyLevel.h"
#include "ActiveGraphicsItemGroup.h"
#include "GammaTransition.h"
#include "GraphicsHighlightItem.h"
#include "ui_Kaihen.h"
#include "Kaihen.h"

#include "ENSDFMassChain.h" /// \todo remove!

const double Decay::outerGammaMargin = 50.0;
const double Decay::outerLevelTextMargin = 4.0; // level lines extend beyond the beginning/end of the level texts by this value
const double Decay::maxExtraLevelDistance = 120.0;
const double Decay::levelToHalfLifeDistance = 10.0;
const double Decay::parentNuclideLevelLineLength = 110.0;
const double Decay::parentNuclideLevelLineExtraLength = 15.5;
const double Decay::parentNuclideMinSpinEnergyDistance = 15.0;
const double Decay::arrowHeadLength = 11.0;
const double Decay::arrowHeadWidth = 5.0;
const double Decay::arrowGap = 5.0;
const double Decay::parentNuclideToEnergyLevelsDistance = 30.0;
const double Decay::highlightWidth = 5.0;


Decay::Decay(const QString &name, Nuclide *parentNuclide, Nuclide *daughterNuclide, Type decayType, QObject *parent)
    : QObject(parent), m_name(name), pNuc(parentNuclide), dNuc(daughterNuclide), t(decayType),
      scene(0), ui(0),
      pNucVerticalArrow(0), pNucHl(0),
      firstSelectedGamma(0), secondSelectedGamma(0), selectedEnergyLevel(0)
{
}

Decay::~Decay()
{
    delete pNuc;
    delete dNuc;
}

QString Decay::decayTypeAsText(Type type)
{
    switch (type) {
    case ElectronCapture:
        return "Electron Capture";
    case BetaPlus:
        return QString::fromUtf8("β+");
    case BetaMinus:
        return QString::fromUtf8("β-");
    case IsomericTransition:
        return "Isomeric Transition";
    case Alpha:
        return QString::fromUtf8("α");
    default:
        break;
    }
    return "";
}

QGraphicsScene * Decay::levelPlot()
{
    if (!scene) {

        scene = new QGraphicsScene(this);

        // decide if parent nuclide should be printed on the left side (beta-),
        // on the right side (EC, beta+, alpha) or not at all (isomeric)
        enum ParentPosition {
            NoParent,
            LeftParent,
            RightParent
        } parentpos = RightParent;
        if (t == IsomericTransition)
            parentpos = NoParent;
        else if (t == BetaMinus)
            parentpos = LeftParent;

        // create level items
        foreach (EnergyLevel *level, dNuc->levels()) {
            QFontMetrics stdBoldFontMetrics(stdBoldFont);

            level->item = new ActiveGraphicsItemGroup(level);
            connect(this, SIGNAL(enableShadow(bool)), level->item, SLOT(setShadowEnabled(bool)));
            level->item->setActiveColor(QColor(224, 186, 100, 180));
            connect(level->item, SIGNAL(clicked(ClickableItem*)), this, SLOT(itemClicked(ClickableItem*)));

            level->graline = new QGraphicsLineItem(-outerGammaMargin, 0.0, outerGammaMargin, 0.0, level->item);
            level->graline->setPen(levelPen);
            // thick line for stable/isomeric levels
            if (level->halfLife().isStable() || level->isomerNum() > 0)
                level->graline->setPen(stableLevelPen);

            level->graclickarea = new QGraphicsRectItem(-outerGammaMargin, -0.5*stdBoldFontMetrics.height(), 2.0*outerGammaMargin, stdBoldFontMetrics.height());
            level->graclickarea->setPen(Qt::NoPen);
            level->graclickarea->setBrush(Qt::NoBrush);

            level->grahighlighthelper = new GraphicsHighlightItem(-outerGammaMargin, -0.5*highlightWidth, 2.0*outerGammaMargin, highlightWidth);
            level->grahighlighthelper->setOpacity(0.0);

            QString etext = level->energyAsText();
            level->graetext = new QGraphicsSimpleTextItem(etext, level->item);
            level->graetext->setFont(stdBoldFont);
            level->graetext->setPos(0.0, -stdBoldFontMetrics.height());

            QString spintext = level->spin().toString();
            level->graspintext = new QGraphicsSimpleTextItem(spintext, level->item);
            level->graspintext->setFont(stdBoldFont);
            level->graspintext->setPos(0.0, -stdBoldFontMetrics.height());

            QString hltext = level->halfLife().toString();
            level->grahltext = new QGraphicsSimpleTextItem(hltext, level->item);
            level->grahltext->setFont(stdFont);
            level->grahltext->setPos(0.0, -0.5*stdBoldFontMetrics.height());

            level->item->addHighlightHelper(level->grahighlighthelper);
            level->item->addToGroup(level->graline);
            level->item->addToGroup(level->graclickarea);
            level->item->addToGroup(level->graetext);
            level->item->addToGroup(level->graspintext);
            level->item->addToGroup(level->grahltext);
            scene->addItem(level->item);

            // plot level feeding arrow if necessary
            double feedintensity = level->normalizedFeedIntensity();
            if (std::isfinite(feedintensity)) {
                // create line
                level->grafeedarrow = new QGraphicsLineItem;
                level->grafeedarrow->setPen((level->feedintens >= 10.0) ? intenseFeedArrowPen : feedArrowPen);
                scene->addItem(level->grafeedarrow);
                // create arrow head
                QPolygonF arrowpol;
                arrowpol << QPointF(0.0, 0.0);
                arrowpol << QPointF((parentpos == RightParent ? 1.0 : -1.0) * arrowHeadLength, 0.5*arrowHeadWidth);
                arrowpol << QPointF((parentpos == RightParent ? 1.0 : -1.0) * arrowHeadLength, -0.5*arrowHeadWidth);
                level->graarrowhead = new QGraphicsPolygonItem(arrowpol);
                level->graarrowhead->setBrush(QColor(level->grafeedarrow->pen().color()));
                level->graarrowhead->setPen(Qt::NoPen);
                scene->addItem(level->graarrowhead);
                // create intensity label
                level->grafeedintens = new QGraphicsSimpleTextItem(QString("%1 %").arg(feedintensity));
                level->grafeedintens->setFont(feedIntensityFont);
                scene->addItem(level->grafeedintens);
            }
        }

        // create gammas
        foreach (EnergyLevel *level, dNuc->levels()) {
            QList<GammaTransition*> levelgammas = level->depopulatingTransitions();
            foreach (GammaTransition *gamma, levelgammas) {
                ActiveGraphicsItemGroup *item = gamma->createGammaGraphicsItem(gammaFont, gammaPen, intenseGammaPen);
                connect(this, SIGNAL(enableShadow(bool)), item, SLOT(setShadowEnabled(bool)));
                connect(item, SIGNAL(clicked(ClickableItem*)), this, SLOT(itemClicked(ClickableItem*)));
                scene->addItem(item);
            }
        }

        // create parent nuclide label and level(s)
        if (parentpos == LeftParent || parentpos == RightParent) {
            // create nuclide label
            QGraphicsItem *pNucGra = pNuc->createNuclideGraphicsItem(nucFont, nucIndexFont);
            scene->addItem(pNucGra);

            // create half-life label
            pNucHl = new QGraphicsSimpleTextItem(pNuc->halfLifeAsText());
            pNucHl->setFont(parentHlFont);
            scene->addItem(pNucHl);

            // create vertical arrow component
            pNucVerticalArrow = new QGraphicsLineItem;
            pNucVerticalArrow->setPen(feedArrowPen);
            scene->addItem(pNucVerticalArrow);

            // create level items
            foreach (EnergyLevel *level, pNuc->levels()) {
                QFontMetrics stdBoldFontMetrics(stdBoldFont);

                level->graline = new QGraphicsLineItem;
                level->graline->setPen(stableLevelPen);
                scene->addItem(level->graline);

                level->graetext = new QGraphicsSimpleTextItem(level->energyAsText());
                level->graetext->setFont(stdBoldFont);
                scene->addItem(level->graetext);

                level->graspintext = new QGraphicsSimpleTextItem(level->spin().toString());
                level->graspintext->setFont(stdBoldFont);
                scene->addItem(level->graspintext);
            }

        }

        // create daughter nuclide label
        QGraphicsItem *dNucGra = dNuc->createNuclideGraphicsItem(nucFont, nucIndexFont);
        scene->addItem(dNucGra);

        alignGraphicsItems();
    }

    return scene;
}

void Decay::setUpdateableUi(Ui::KaihenMainWindow *updui)
{
    ui = updui;
}

void Decay::setShadowEnabled(bool enable)
{
    emit enableShadow(enable);
}

QString Decay::name() const
{
    return m_name;
}

QVector<QPointF> Decay::gammaSpectrum(double fwhm) const
{
    // collect gammas
    QMap<double, double> gammas;
    foreach (EnergyLevel *level, dNuc->levels())
        foreach (GammaTransition *g, level->depopulatingTransitions())
            if (std::isfinite(g->intensity()))
                gammas.insert(g->energyKeV(), g->intensity());

    // determine highest energy
    double max = 0.0;
    if (!gammas.isEmpty())
        max = (gammas.end()-1).key();

    // determine sigma @ 662 keV
    double sigma = fwhm / (2.0*sqrt(2.0*M_LN2));
    double var = sigma * sigma;

    // create result vector (on interval [0, max+2*fwhm], stepwidth: 2keV)
    QVector<QPointF> result(std::ceil(max/2.) + qRound(2.0*fwhm));

    // compute values
    for (int i=0; i<result.size(); i++) {
        result[i].setX(double(i*2+1));
        for (QMap<double, double>::const_iterator gamma=gammas.begin(); gamma!=gammas.end(); gamma++) {
            double eGamma = gamma.key();
            result[i].ry() += gamma.value() * gauss(result[i].x() - eGamma, sqrt(eGamma/662.0*var));
        }
    }

    return result;
}

void Decay::itemClicked(ClickableItem *item)
{
    if (item->type() == ClickableItem::EnergyLevelType)
        clickedEnergyLevel(dynamic_cast<EnergyLevel*>(item));
    else if (item->type() == ClickableItem::GammaTransitionType)
        clickedGamma(dynamic_cast<GammaTransition*>(item));
}

void Decay::clickedGamma(GammaTransition *g)
{
    if (!g)
        return;

    // deselect if active level is clicked again
    if (g == firstSelectedGamma) {
        firstSelectedGamma->graphicsItem()->setHighlighted(false);
        firstSelectedGamma = secondSelectedGamma;
        secondSelectedGamma = 0;
    }
    else if (g == secondSelectedGamma) {
        secondSelectedGamma->graphicsItem()->setHighlighted(false);
        secondSelectedGamma = 0;
    }
    else {
        // deselect inappropriate level(s)
        bool firstok = false;
        if (firstSelectedGamma)
            firstok = g->populatedLevel() == firstSelectedGamma->depopulatedLevel() ||
                    g->depopulatedLevel() == firstSelectedGamma->populatedLevel();
        bool secondok = false;
        if (secondSelectedGamma)
            secondok = g->populatedLevel() == secondSelectedGamma->depopulatedLevel() ||
                    g->depopulatedLevel() == secondSelectedGamma->populatedLevel();

        if (firstok && secondok) {
            firstSelectedGamma->graphicsItem()->setHighlighted(false);
            firstSelectedGamma = secondSelectedGamma;
            secondSelectedGamma = 0;
        }
        else if (firstok) {
            if (secondSelectedGamma) {
                secondSelectedGamma->graphicsItem()->setHighlighted(false);
                secondSelectedGamma = 0;
            }
        }
        else if (secondok) {
            Q_ASSERT(firstSelectedGamma);
            firstSelectedGamma->graphicsItem()->setHighlighted(false);
            firstSelectedGamma = secondSelectedGamma;
            secondSelectedGamma = 0;
        }
        else {
            if (firstSelectedGamma) {
                firstSelectedGamma->graphicsItem()->setHighlighted(false);
                firstSelectedGamma = 0;
            }
            if (secondSelectedGamma) {
                secondSelectedGamma->graphicsItem()->setHighlighted(false);
                secondSelectedGamma = 0;
            }
        }

        // case: no gamma previously selected
        if (!firstSelectedGamma && !secondSelectedGamma) {
            firstSelectedGamma = g;
            firstSelectedGamma->graphicsItem()->setHighlighted(true);
            if (selectedEnergyLevel) {
                if (g->populatedLevel() != selectedEnergyLevel && g->depopulatedLevel() != selectedEnergyLevel) {
                    selectedEnergyLevel->graphicsItem()->setHighlighted(false);
                    selectedEnergyLevel = 0;
                }
            }
        }
        // case: one gamma previously selected, common level exists
        else {
            secondSelectedGamma = g;
            secondSelectedGamma->graphicsItem()->setHighlighted(true);
            // select intermediate level
            EnergyLevel *intermediate = firstSelectedGamma->populatedLevel();
            if (firstSelectedGamma->depopulatedLevel() == secondSelectedGamma->populatedLevel())
                intermediate = firstSelectedGamma->depopulatedLevel();
            // activate intermediate level
            if (selectedEnergyLevel)
                if (selectedEnergyLevel != intermediate)
                    selectedEnergyLevel->graphicsItem()->setHighlighted(false);
            selectedEnergyLevel = intermediate;
            selectedEnergyLevel->graphicsItem()->setHighlighted(true);
        }
    }

    updateDecayDataLabels();
}

void Decay::clickedEnergyLevel(EnergyLevel *e)
{
    if (!e)
        return;

    // deselect if clicked again
    if (e == selectedEnergyLevel) {
        selectedEnergyLevel->graphicsItem()->setHighlighted(false);
        selectedEnergyLevel = 0;
    }
    // select otherwise
    else {
        // deselect old level
        if (selectedEnergyLevel)
            selectedEnergyLevel->graphicsItem()->setHighlighted(false);
        selectedEnergyLevel = e;
        e->graphicsItem()->setHighlighted(true);
        // deselect gamma which is not connected to the level anymore
        if (firstSelectedGamma) {
            if (firstSelectedGamma->depopulatedLevel() != e && firstSelectedGamma->populatedLevel() != e) {
                firstSelectedGamma->graphicsItem()->setHighlighted(false);
                firstSelectedGamma = 0;
            }
        }
        if (secondSelectedGamma) {
            if (secondSelectedGamma->depopulatedLevel() != e && secondSelectedGamma->populatedLevel() != e) {
                secondSelectedGamma->graphicsItem()->setHighlighted(false);
                secondSelectedGamma = 0;
            }
        }
        if (secondSelectedGamma && !firstSelectedGamma) {
            firstSelectedGamma = secondSelectedGamma;
            secondSelectedGamma = 0;
        }
    }
    // prevent two gammas being active after the intermediate level was changed
    if (firstSelectedGamma && secondSelectedGamma) {
        firstSelectedGamma->graphicsItem()->setHighlighted(false);
        firstSelectedGamma = secondSelectedGamma;
        secondSelectedGamma = 0;
    }

    updateDecayDataLabels();
}

void Decay::updateDecayDataLabels()
{
    if (!ui)
        return;

    // intermediate level
    if (selectedEnergyLevel) {
        ui->intEnergy->setText(selectedEnergyLevel->energyAsText());
        ui->intHalfLife->setText(selectedEnergyLevel->halfLife().toString());
        ui->intSpin->setText(selectedEnergyLevel->spin().toString());
        ui->intMu->setText(selectedEnergyLevel->muAsText());
        ui->intQ->setText(selectedEnergyLevel->qAsText());
    }
    else {
        ui->intEnergy->setText("- keV");
        ui->intHalfLife->setText("- ns");
        ui->intSpin->setText("/");
        ui->intMu->setText("-");
        ui->intQ->setText("-");
    }

    // gammas
    GammaTransition *pop = 0, *depop = 0;
    if (firstSelectedGamma) {
        if (firstSelectedGamma->depopulatedLevel() == selectedEnergyLevel)
            depop = firstSelectedGamma;
        else
            pop = firstSelectedGamma;
    }
    if (secondSelectedGamma) {
        if (secondSelectedGamma->depopulatedLevel() == selectedEnergyLevel)
            depop = secondSelectedGamma;
        else
            pop = secondSelectedGamma;
    }

    // populating
    if (pop) {
        ui->popEnergy->setText(pop->energyAsText());
        ui->popIntensity->setText(pop->intensityAsText());
        ui->popMultipolarity->setText(pop->multipolarityAsText());
        ui->popMixing->setText(pop->deltaAsText());
    }
    else {
        ui->popEnergy->setText("- keV");
        ui->popIntensity->setText("- %");
        ui->popMultipolarity->setText("<i>unknown</i>");
        ui->popMixing->setText("<i>unknown</i>");
    }

    // depopulating
    if (depop) {
        ui->depopEnergy->setText(depop->energyAsText());
        ui->depopIntensity->setText(depop->intensityAsText());
        ui->depopMultipolarity->setText(depop->multipolarityAsText());
        ui->depopMixing->setText(depop->deltaAsText());
    }
    else {
        ui->depopEnergy->setText("- keV");
        ui->depopIntensity->setText("- %");
        ui->depopMultipolarity->setText("<i>unknown</i>");
        ui->depopMixing->setText("<i>unknown</i>");
    }

    // start and end level
    if (firstSelectedGamma && secondSelectedGamma) {
        ui->startEnergy->setText(pop->depopulatedLevel()->energyAsText());
        ui->startSpin->setText(pop->depopulatedLevel()->spin().toString());
        ui->endEnergy->setText(depop->populatedLevel()->energyAsText());
        ui->endSpin->setText(depop->populatedLevel()->spin().toString());
    }
    else {
        ui->startEnergy->setText("- keV");
        ui->startSpin->setText("/");
        ui->endEnergy->setText("- keV");
        ui->endSpin->setText("/");
    }

    // calculate anisotropies
    if (pop && depop && selectedEnergyLevel) {
        if (pop->depopulatedLevel()->spin().isValid() &&
            depop->populatedLevel()->spin().isValid() &&
            selectedEnergyLevel->spin().isValid() &&
            pop->deltaState() & GammaTransition::SignMagnitudeDefined &&
            depop->deltaState() & GammaTransition::SignMagnitudeDefined
           ) {
            // create list of sign combinations
            typedef QPair<double, double> SignCombination;
            QList< SignCombination > variants;
            QList< double > popvariants;
            if (pop->deltaState() == GammaTransition::MagnitudeDefined)
                popvariants << 1. << -1.;
            else
                popvariants << ((pop->delta() < 0.0) ? -1. : 1.);
            foreach (double popvariant, popvariants) {
                if (depop->deltaState() == GammaTransition::MagnitudeDefined)
                    variants << QPair<double, double>(popvariant, 1.) << SignCombination(popvariant, -1.);
                else
                    variants << SignCombination(popvariant, (depop->delta() < 0.0) ? -1. : 1.);
            }

            // compute possible results
            QStringList a22, a24, a42, a44;
            Akk calc;
            calc.setInitialStateSpin(pop->depopulatedLevel()->spin().doubledSpin());
            calc.setIntermediateStateSpin(selectedEnergyLevel->spin().doubledSpin());
            calc.setFinalStateSpin(depop->populatedLevel()->spin().doubledSpin());

            foreach (SignCombination variant, variants) {
                double popdelta = pop->delta();
                double depopdelta = depop->delta();
                if (pop->deltaState() == GammaTransition::MagnitudeDefined)
                    popdelta *= variant.first;
                if (depop->deltaState() == GammaTransition::MagnitudeDefined)
                    depopdelta *= variant.second;

                calc.setPopulatingGammaMixing(popdelta);
                calc.setDepopulatingGammaMixing(depopdelta);

                // determine prefix
                QString prfx;
                if (variants.size() > 1) {
                    prfx = "%1: ";
                    prfx = prfx.arg(QString(variant.first < 0 ? "-" : "+") + QString(variant.second < 0 ? "-" : "+"));
                }

                a22.append(QString("%1%2").arg(prfx).arg(calc.a22()));
                a24.append(QString("%1%2").arg(prfx).arg(calc.a24()));
                a42.append(QString("%1%2").arg(prfx).arg(calc.a42()));
                a44.append(QString("%1%2").arg(prfx).arg(calc.a44()));
            }

            ui->a22->setText(a22.join(", "));
            ui->a24->setText(a24.join(", "));
            ui->a42->setText(a42.join(", "));
            ui->a44->setText(a44.join(", "));
        }
        else {
            resetAnisotropyLabels();
        }
    }
    else {
        resetAnisotropyLabels();
    }
}

void Decay::resetAnisotropyLabels()
{
    if (!ui)
        return;

    ui->a22->setText("-");
    ui->a24->setText("-");
    ui->a42->setText("-");
    ui->a44->setText("-");
}

void Decay::alignGraphicsItems()
{
    // decide if parent nuclide should be printed on the left side (beta-),
    // on the right side (EC, beta+, alpha) or not at all (isomeric)
    enum ParentPosition {
        NoParent,
        LeftParent,
        RightParent
    } parentpos = RightParent;
    if (t == IsomericTransition)
        parentpos = NoParent;
    else if (t == BetaMinus)
        parentpos = LeftParent;

    QFontMetrics stdFontMetrics(stdFont);
    QFontMetrics stdBoldFontMetrics(stdBoldFont);
    QFontMetrics parentHlFontMetrics(parentHlFont);
    QFontMetrics feedIntensityFontMetrics(feedIntensityFont);

    // determine size information
    double maxEnergyLabelWidth = 0.0;
    double maxSpinLabelWidth = 0.0;

    foreach (EnergyLevel *level, dNuc->levels()) {
        if (stdBoldFontMetrics.width(level->graspintext->text()) > maxSpinLabelWidth)
            maxSpinLabelWidth = stdBoldFontMetrics.width(level->graspintext->text());
        if (stdBoldFontMetrics.width(level->graetext->text()) > maxEnergyLabelWidth)
            maxEnergyLabelWidth = stdBoldFontMetrics.width(level->graetext->text());
    }

    // determine y coordinates for all levels
    double maxEnergyGap = 0.0;
    QMap<double, EnergyLevel*> levels(dNuc->levels());
    for (QMap<double, EnergyLevel*>::const_iterator i=levels.begin()+1; i!=levels.end(); i++) {
        double diff = i.key() - (i-1).key();
        maxEnergyGap = qMax(maxEnergyGap, diff);
    }
    for (QMap<double, EnergyLevel*>::const_iterator i=levels.begin()+1; i!=levels.end(); i++) {
        double minheight = 0.5*(i.value()->item->boundingRect().height() + (i-1).value()->item->boundingRect().height());
        double extraheight = maxExtraLevelDistance * (i.key() - (i-1).key()) / maxEnergyGap;
        i.value()->graYPos = std::floor((i-1).value()->graYPos - minheight - extraheight) + 0.5*i.value()->graline->pen().widthF();
    }

    // determine space needed for gammas
    double gammaspace = std::numeric_limits<double>::quiet_NaN();
    foreach (EnergyLevel *level, dNuc->levels()) {
        QList<GammaTransition*> levelgammas = level->depopulatingTransitions();
        foreach (GammaTransition *gamma, levelgammas) {
            if (std::isnan(gammaspace))
                gammaspace = gamma->widthFromOrigin();
            else
                gammaspace += gamma->minimalXDistance();
        }
    }
    if (!std::isfinite(gammaspace))
        gammaspace = 0.0;

    // set gamma positions
    double currentgammapos = 0.5*gammaspace;
    bool firstgamma = true;
    foreach (EnergyLevel *level, levels) {
        QList<GammaTransition*> levelgammas = level->depopulatingTransitions();
        foreach (GammaTransition *gamma, levelgammas) {
            if (firstgamma) {
                currentgammapos -= gamma->widthFromOrigin();
                firstgamma = false;
            }
            else {
                currentgammapos -= gamma->minimalXDistance();
            }
            gamma->updateArrow();
            gamma->graphicsItem()->setPos(std::floor(currentgammapos)+0.5*gamma->pen().widthF(), level->graYPos + 0.5*level->graline->pen().widthF());
        }
    }

    // determine line length for parent levels
    double pNucLineLength = parentNuclideLevelLineLength;
    if (parentpos == LeftParent || parentpos == RightParent) {
        pNucLineLength = qMax(parentNuclideLevelLineLength, pNuc->nuclideGraphicsItem()->boundingRect().width() + 20.0);
        foreach (EnergyLevel *level, pNuc->levels()) {
            pNucLineLength = qMax(pNucLineLength,
                                  level->graetext->boundingRect().width() +
                                  level->graspintext->boundingRect().width() +
                                  parentNuclideMinSpinEnergyDistance +
                                  2.0 * outerLevelTextMargin);
        }
    }
    pNucLineLength = std::ceil(pNucLineLength);

    // calculate length of level lines
    double leftlinelength = outerLevelTextMargin + maxSpinLabelWidth + outerGammaMargin + 0.5*gammaspace;
    double rightlinelength = outerLevelTextMargin + maxEnergyLabelWidth + outerGammaMargin + 0.5*gammaspace;

    // calculate start and end points of parent level lines
    double normalleft = std::floor((parentpos == RightParent) ? rightlinelength : -leftlinelength - pNucLineLength) - 0.5*feedArrowPen.widthF();
    double normalright = std::ceil((parentpos == RightParent) ? rightlinelength + pNucLineLength : -leftlinelength) + 0.5*feedArrowPen.widthF();
    double activeleft = std::floor((parentpos == RightParent) ? normalleft : normalleft - parentNuclideLevelLineExtraLength) - 0.5*feedArrowPen.widthF();
    double activeright = std::ceil((parentpos == RightParent) ? normalright + parentNuclideLevelLineExtraLength : normalright) + 0.5*feedArrowPen.widthF();

    // set level positions and sizes
    double arrowVEnd = std::numeric_limits<double>::quiet_NaN();
    foreach (EnergyLevel *level, levels) {

        // temporarily remove items from group (workaround)
        level->item->removeFromGroup(level->grahltext);
        level->item->removeFromGroup(level->graspintext);
        level->item->removeFromGroup(level->graetext);
        level->item->removeFromGroup(level->graclickarea);
        level->item->removeFromGroup(level->graline);
        level->item->removeHighlightHelper(level->grahighlighthelper);

        // rescale
        level->grahighlighthelper->setRect(-leftlinelength, -0.5*highlightWidth, leftlinelength+rightlinelength, highlightWidth);
        level->graline->setLine(-leftlinelength, 0.0, rightlinelength, 0.0);
        level->graclickarea->setRect(-leftlinelength, -0.5*stdBoldFontMetrics.height(), leftlinelength+rightlinelength, stdBoldFontMetrics.height());
        level->graspintext->setPos(-leftlinelength + outerLevelTextMargin, -stdBoldFontMetrics.height());
        level->graetext->setPos(rightlinelength - outerLevelTextMargin - stdBoldFontMetrics.width(level->graetext->text()), -stdBoldFontMetrics.height());
        double levelHlPos = 0.0;
        if (parentpos == RightParent) {
            levelHlPos = -leftlinelength - levelToHalfLifeDistance - stdFontMetrics.width(level->grahltext->text());
        }
        else {
            levelHlPos = rightlinelength + levelToHalfLifeDistance;
        }
        level->grahltext->setPos(levelHlPos, -0.5*stdBoldFontMetrics.height());

        // re-add items to group
        level->item->addHighlightHelper(level->grahighlighthelper);
        level->item->addToGroup(level->graline);
        level->item->addToGroup(level->graclickarea);
        level->item->addToGroup(level->graetext);
        level->item->addToGroup(level->graspintext);
        level->item->addToGroup(level->grahltext);

        level->item->setPos(0.0, level->graYPos); // add 0.5*pen-width to avoid antialiasing artifacts

        if (level->grafeedarrow) {
            double leftend = (parentpos == RightParent) ? rightlinelength + arrowGap + arrowHeadLength : activeleft;
            double rightend = (parentpos == RightParent) ? activeright : -leftlinelength - arrowGap - arrowHeadLength;
            double arrowY = level->graYPos;
            level->grafeedarrow->setLine(leftend, arrowY, rightend, arrowY);
            level->graarrowhead->setPos((parentpos == RightParent) ? rightlinelength + arrowGap : -leftlinelength - arrowGap, arrowY);
            if (std::isnan(arrowVEnd))
                arrowVEnd = arrowY + 0.5*level->grafeedarrow->pen().widthF();
            level->grafeedintens->setPos(leftend + 15.0, arrowY - feedIntensityFontMetrics.height());
        }
    }

    // set position of daughter nuclide
    dNuc->nuclideGraphicsItem()->setPos(-0.5*dNuc->nuclideGraphicsItem()->boundingRect().width(), 0.3*dNuc->nuclideGraphicsItem()->boundingRect().height());

    // set position of parent nuclide
    if (parentpos == LeftParent || parentpos == RightParent) {
        double maxEnergy = (levels.end()-1).key();
        double parentY = std::numeric_limits<double>::quiet_NaN();
        if (!levels.isEmpty())
            parentY = levels.value(maxEnergy)->item->y() -
                    levels.value(maxEnergy)->item->boundingRect().height() -
                    pNuc->nuclideGraphicsItem()->boundingRect().height() - parentNuclideToEnergyLevelsDistance;

        double parentcenter;
        if (parentpos == RightParent)
            parentcenter = rightlinelength + 0.5*(pNucLineLength+parentNuclideLevelLineExtraLength);
        else
            parentcenter = -leftlinelength - 0.5*(pNucLineLength+parentNuclideLevelLineExtraLength);

        pNuc->nuclideGraphicsItem()->setPos(parentcenter - 0.5*pNuc->nuclideGraphicsItem()->boundingRect().width(), parentY);

        // set position of parent levels
        double topMostLevel = 0.0;
        double y = qRound(parentY - 0.3*pNuc->nuclideGraphicsItem()->boundingRect().height()) + 0.5*stableLevelPen.widthF();
        foreach (EnergyLevel *level, pNuc->levels()) {
            double left = level->isFeedingLevel() ? activeleft : normalleft;
            double right = level->isFeedingLevel() ? activeright : normalright;

            level->graline->setLine(left, y, right, y);
            level->graetext->setPos((pNuc->levels().size() == 1 ? activeright : normalright) - outerLevelTextMargin - level->graetext->boundingRect().width(), y - level->graetext->boundingRect().height());
            level->graspintext->setPos((pNuc->levels().size() == 1 ? activeleft : normalleft) + outerLevelTextMargin, y - level->graetext->boundingRect().height());

            topMostLevel = y;

            // update y
            y -= qMax(level->graetext->boundingRect().height(), level->graspintext->boundingRect().height()) + 10.0;
        }

        double arrowVStart = topMostLevel - 0.5*stableLevelPen.widthF();
        double arrowX = (parentpos == RightParent) ? activeright : activeleft;
        if (std::isfinite(arrowVStart) && std::isfinite(arrowVEnd))
            pNucVerticalArrow->setLine(arrowX, arrowVStart, arrowX, arrowVEnd);

        pNucHl->setPos(parentcenter - 0.5*pNucHl->boundingRect().width(), topMostLevel - stdBoldFontMetrics.height() - parentHlFontMetrics.height() - 12.0);
    }
}

void Decay::setStyle(const QFont &fontfamily, unsigned int sizePx)
{
    // prepare fonts and their metrics
    stdFont = fontfamily;
    stdFont.setPixelSize(sizePx);
    stdBoldFont = fontfamily;
    stdBoldFont.setPixelSize(sizePx);
    stdBoldFont.setBold(true);
    nucFont = fontfamily;
    nucFont.setPixelSize(sizePx * 20 / 10);
    nucFont.setBold(true);
    nucIndexFont = fontfamily;
    nucIndexFont.setPixelSize(sizePx * 12 / 10);
    nucIndexFont.setBold(true);
    parentHlFont = fontfamily;
    parentHlFont.setPixelSize(sizePx * 12 / 10);
    feedIntensityFont = fontfamily;
    feedIntensityFont.setPixelSize(sizePx);
    feedIntensityFont.setItalic(true);
    gammaFont = fontfamily;
    gammaFont.setPixelSize(sizePx);

    // prepare pens
    levelPen.setWidthF(1.0);
    levelPen.setCapStyle(Qt::FlatCap);
    stableLevelPen.setWidthF(2.0);
    stableLevelPen.setCapStyle(Qt::FlatCap);
    feedArrowPen.setWidthF(1.0);
    feedArrowPen.setCapStyle(Qt::FlatCap);
    intenseFeedArrowPen.setWidthF(2.0);
    intenseFeedArrowPen.setColor(QColor(232, 95, 92));
    intenseFeedArrowPen.setCapStyle(Qt::FlatCap);
    gammaPen.setWidthF(1.0);
    gammaPen.setCapStyle(Qt::FlatCap);
    intenseGammaPen.setWidthF(2.0);
    intenseGammaPen.setColor(QColor(232, 95, 92));
    intenseGammaPen.setCapStyle(Qt::FlatCap);
}

double Decay::gauss(const double x, const double sigma) const
{
    double u = x / fabs(sigma);
    double p = (1 / (sqrt(2 * M_PI) * fabs(sigma))) * exp(-u * u / 2);
    return p;
}

