#ifndef JETBACKGROUND_SUPERTOWERRECO_H
#define JETBACKGROUND_SUPERTOWERRECO_H

#include <fun4all/SubsysReco.h>

#include <string>

class PHCompositeNode;

class SuperTowerReco : public SubsysReco
{
 public:
  SuperTowerReco(const std::string &name = "SuperTowerReco");
  ~SuperTowerReco() override = default;

  int InitRun(PHCompositeNode *topNode) override;
  int process_event(PHCompositeNode *topNode) override;

  void set_towerNodePrefix(const std::string &prefix)
  {
    m_towerNodePrefix = prefix;
    return;
  }

 private:
  int CreateNode(PHCompositeNode *topNode);

  std::string m_towerNodePrefix{"TOWERINFO_CALIB"};

  static const int neta_ihcal = 24;
  static const int neta_emcal = 96;
  static const int nphi_ihcal = 64;
  static const int nphi_emcal = 256;

  double rawtower_e[neta_emcal][nphi_emcal] = {{0.0}};
  double rawtower_time[neta_emcal][nphi_emcal] = {{0.0}};
  int rawtower_status[neta_emcal][nphi_emcal] = {{0}};

  std::string EMTowerName;
  std::string IHTowerName;
  std::string OHTowerName;
  std::string SuperTowerName;
};

#endif
