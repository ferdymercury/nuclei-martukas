#ifndef ENSDFMASSCHAIN_H
#define ENSDFMASSCHAIN_H
#include <QStringList>
#include <QSharedPointer>
#include <QPair>
#include "Decay.h"
#include "Energy.h"

class HalfLife;

class ENSDFMassChain
{
public:
    explicit ENSDFMassChain(unsigned int A);

    static const QList<unsigned int> &aValues();

    unsigned int aValue() const;
    const QStringList & daughterNuclides() const;
    const QStringList & decays(const QString &daughterNuclideName) const;

    /**
     * @brief decay
     * @param daughterNuclideName
     * @param decayName
     * @param adoptedLevelMaxDifference Maximal acceptable difference between energy levels (in eV) in decay and adopted level blocks
     * @param gammaMaxDifference Maximal difference between the energy of gammas in decay records and adopted levels
     * @return
     */
    QSharedPointer<Decay> decay(const QString &daughterNuclideName, const QString &decayName);

    static unsigned int A(const QString &nuclide);

private:
    typedef QPair<int,int> BlockIndices; // [startidx, size]

    struct ParentRecord {
        QString nuclideName;
        Energy energy;
        HalfLife hl;
        SpinParity spin;

    };

    struct BasicDecayData {
        QList<ParentRecord> parents;
        QString daughterName;
        Decay::Type decayType;
        BlockIndices block;
    };

    static QList<unsigned int> aList;

    static QString nuclideToNucid(const QString &nuclide);
    static QString nucidToNuclide(const QString &nucid);
    static QString element(const QString &nuclide);
    static Decay::Type parseDecayType(const QString &tstring);
    static Energy parseEnsdfEnergy(const QString &estr);
    static HalfLife parseHalfLife(const QString &hlstr);
    static SpinParity parseSpinParity(const QString &sstr);
    static double parseEnsdfMixing(const QString &s, const QString &multipolarity, GammaTransition::DeltaState *state);
    static ParentRecord parseParentRecord(const QString &precstr);

    template <typename T> static T & findNearest(QMap<Energy, T> &map, Energy val, Energy *foundVal = 0);

    void parseBlocks();

    const unsigned int a;

    QStringList contents;
    QMap< QString, BlockIndices > m_adoptedlevels; // daughter name: [startidx, size]
    QMap< QString, QMap<QString, BasicDecayData > > m_decays; // daughter name: (decay name: [startidx, size])
    QStringList m_daughterNames;
    mutable QMap< QString, QStringList > m_decayNames;
};

#endif // ENSDF_H
