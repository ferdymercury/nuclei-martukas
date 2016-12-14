#ifndef XNuclide_H
#define XNuclide_H

#include <vector>
#include <limits>
#include <memory>
#include "NuclideId.h"
#include "Level.h"
#include "Transition.h"

class Nuclide
{
public:
  Nuclide();
  Nuclide(NuclideId id);

  NuclideId id() const;

  bool empty() const;

  void addLevels(const std::map<Energy, Level> &levels);
  void addTransitions(const std::map<Energy, Transition> &tr);

  void addLevel(const Level& level);
  void addTransition(const Transition& transition);

  std::map<Energy, Level> levels() const;
  std::map<Energy, Transition> transitions() const;

  void setHalflives(const std::vector<HalfLife>& hl);
  void addHalfLife(const HalfLife& hl);

  std::vector<HalfLife> halfLifes() const;
  std::string halfLifeAsText() const;

private:
  NuclideId   id_;
  std::vector <HalfLife> halflives_;
  std::map<Energy, Level> levels_;
  std::map<Energy, Transition> transitions_;
};

#endif
