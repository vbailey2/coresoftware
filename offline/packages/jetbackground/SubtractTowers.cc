#include "SubtractTowers.h"

#include "TowerBackground.h"

// sPHENIX includes
#include <calobase/RawTower.h>
#include <calobase/RawTowerContainer.h>
#include <calobase/RawTowerDefs.h>
#include <calobase/RawTowerGeom.h>
#include <calobase/RawTowerGeomContainer.h>
#include <calobase/RawTowerv1.h>

#include <calobase/TowerInfo.h>
#include <calobase/TowerInfoContainer.h>

#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/SubsysReco.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/PHNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHObject.h>
#include <phool/getClass.h>
#include <phool/phool.h>

// standard includes
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

SubtractTowers::SubtractTowers(const std::string &name)
  : SubsysReco(name)
{
}

int SubtractTowers::InitRun(PHCompositeNode *topNode)
{
  CreateNode(topNode);

  return Fun4AllReturnCodes::EVENT_OK;
}

int SubtractTowers::process_event(PHCompositeNode *topNode)
{
  if (Verbosity() > 0)
  {
    std::cout << "SubtractTowers::process_event: entering, with _use_flow_modulation = " << _use_flow_modulation << std::endl;
  }

  // pull out the tower containers and geometry objects at the start
  TowerInfoContainer *towerinfosEM3 = nullptr;
  TowerInfoContainer *towerinfosIH3 = nullptr;
  TowerInfoContainer *towerinfosOH3 = nullptr;

  if (m_use_towerinfo)
  {
    if(m_use_retower) EMTowerName = m_towerNodePrefix + "_CEMC_RETOWER";
    else EMTowerName = m_towerNodePrefix + "_CEMC";
    IHTowerName = m_towerNodePrefix + "_HCALIN";
    OHTowerName = m_towerNodePrefix + "_HCALOUT";
    towerinfosEM3 = findNode::getClass<TowerInfoContainer>(topNode, EMTowerName);
    towerinfosIH3 = findNode::getClass<TowerInfoContainer>(topNode, IHTowerName);
    towerinfosOH3 = findNode::getClass<TowerInfoContainer>(topNode, OHTowerName);

    if (Verbosity() > 0)
    {
      std::cout << "SubtractTowers::process_event: " << towerinfosEM3->size() << EMTowerName << " towers" << std::endl;
      std::cout << "SubtractTowers::process_event: " << towerinfosIH3->size() << IHTowerName << " towers" << std::endl;
      std::cout << "SubtractTowers::process_event: " << towerinfosOH3->size() << OHTowerName << " towers" << std::endl;
    }
  }

  RawTowerGeomContainer *geomIH = findNode::getClass<RawTowerGeomContainer>(topNode, "TOWERGEOM_HCALIN");
  RawTowerGeomContainer *geomOH = findNode::getClass<RawTowerGeomContainer>(topNode, "TOWERGEOM_HCALOUT");

  // these should have already been created during InitRun()
  RawTowerContainer *emcal_towers = nullptr;
  RawTowerContainer *ihcal_towers = nullptr;
  RawTowerContainer *ohcal_towers = nullptr;
  TowerInfoContainer *emcal_towerinfos = nullptr;
  TowerInfoContainer *ihcal_towerinfos = nullptr;
  TowerInfoContainer *ohcal_towerinfos = nullptr;
  if (m_use_towerinfo)
  {
    EMTowerName = m_towerNodePrefix + "_CEMC_SUB1";
    if(m_use_retower) EMTowerName = m_towerNodePrefix + "_CEMC_RETOWER_SUB1";
    IHTowerName = m_towerNodePrefix + "_HCALIN_SUB1";
    OHTowerName = m_towerNodePrefix + "_HCALOUT_SUB1";
    emcal_towerinfos = findNode::getClass<TowerInfoContainer>(topNode, EMTowerName);
    ihcal_towerinfos = findNode::getClass<TowerInfoContainer>(topNode, IHTowerName);
    ohcal_towerinfos = findNode::getClass<TowerInfoContainer>(topNode, OHTowerName);
    if (Verbosity() > 0)
      {
	std::cout << "SubtractTowers::process_event: starting with " << emcal_towerinfos->size() << EMTowerName << " towers" << std::endl;
	std::cout << "SubtractTowers::process_event: starting with " << ihcal_towerinfos->size() << IHTowerName << " towers" << std::endl;
	std::cout << "SubtractTowers::process_event: starting with " << ohcal_towerinfos->size() << OHTowerName << " towers" << std::endl;
      }
  }

  TowerBackground *towerbackground = findNode::getClass<TowerBackground>(topNode, "TowerInfoBackground_Sub2");

  // read these in to use, even if we don't use flow modulation in the subtraction
  float background_v2 = towerbackground->get_v2();
  float background_Psi2 = towerbackground->get_Psi2();

  // EMCal

  // replicate existing towers
  if (m_use_towerinfo)
  {
    unsigned int nchannels_em = towerinfosEM3->size();
    for (unsigned int channel = 0; channel < nchannels_em; channel++)
    {
      TowerInfo *tower = towerinfosEM3->get_tower_at_channel(channel);
      unsigned int towerkey = towerinfosEM3->encode_key(channel);
      int ieta = towerinfosEM3->getTowerEtaBin(towerkey);
      int iphi = towerinfosEM3->getTowerPhiBin(towerkey);
      float raw_energy = tower->get_energy();
      float UE = towerbackground->get_UE(0).at(ieta);
      if (_use_flow_modulation)
      {
        const RawTowerDefs::keytype key = RawTowerDefs::encode_towerid(RawTowerDefs::CalorimeterId::HCALIN, ieta, iphi);
        float tower_phi = geomIH->get_tower_geometry(key)->get_phi();
        UE = UE * (1 + 2 * background_v2 * std::cos(2 * (tower_phi - background_Psi2)));
      }
      float new_energy = raw_energy - UE;
      // if a tower is masked, leave it at zero
      if (!tower->get_isGood())
      {
        new_energy = 0;
      }

      emcal_towerinfos->get_tower_at_channel(channel)->set_time(tower->get_time());
      emcal_towerinfos->get_tower_at_channel(channel)->set_energy(new_energy);

      if (Verbosity() > 5)
      {
        std::cout << " SubtractTowers::process_event : EMCal tower at ieta / iphi = " << ieta << " / " << iphi << ", pre-sub / after-sub E = " << raw_energy << " / " << new_energy << std::endl;
      }
    }
  }

  // IHCal
  // replicate existing towers
  if (m_use_towerinfo)
  {
    unsigned int nchannels_ih = towerinfosIH3->size();
    for (unsigned int channel = 0; channel < nchannels_ih; channel++)
    {
      TowerInfo *tower = towerinfosIH3->get_tower_at_channel(channel);
      unsigned int towerkey = towerinfosIH3->encode_key(channel);
      int ieta = towerinfosIH3->getTowerEtaBin(towerkey);
      int iphi = towerinfosIH3->getTowerPhiBin(towerkey);

      float raw_energy = tower->get_energy();
      float UE = towerbackground->get_UE(1).at(ieta);
      if (_use_flow_modulation)
      {
        const RawTowerDefs::keytype key = RawTowerDefs::encode_towerid(RawTowerDefs::CalorimeterId::HCALIN, ieta, iphi);
        float tower_phi = geomIH->get_tower_geometry(key)->get_phi();
        UE = UE * (1 + 2 * background_v2 * std::cos(2 * (tower_phi - background_Psi2)));
      }
      float new_energy = raw_energy - UE;
      // if a tower is masked, leave it at zero
      if (!tower->get_isGood())
      {
        new_energy = 0;
      }

      ihcal_towerinfos->get_tower_at_channel(channel)->set_time(tower->get_time());
      ihcal_towerinfos->get_tower_at_channel(channel)->set_energy(new_energy);
      if (Verbosity() > 5)
      {
        std::cout << "SubtractTowers::process_event : IHCal tower at ieta / iphi = " << ieta << " / " << iphi << ", pre-sub / after-sub E = " << raw_energy << " / " << new_energy << std::endl;
      }
    }
  }

  // OHCal

  // replicate existing towers
  if (m_use_towerinfo)
  {
    unsigned int nchannels_oh = towerinfosOH3->size();
    for (unsigned int channel = 0; channel < nchannels_oh; channel++)
    {
      TowerInfo *tower = towerinfosOH3->get_tower_at_channel(channel);
      unsigned int towerkey = towerinfosOH3->encode_key(channel);
      int ieta = towerinfosOH3->getTowerEtaBin(towerkey);
      int iphi = towerinfosOH3->getTowerPhiBin(towerkey);
      float raw_energy = tower->get_energy();
      float UE = towerbackground->get_UE(2).at(ieta);
      if (_use_flow_modulation)
      {
        const RawTowerDefs::keytype key = RawTowerDefs::encode_towerid(RawTowerDefs::CalorimeterId::HCALOUT, ieta, iphi);
        float tower_phi = geomOH->get_tower_geometry(key)->get_phi();
        UE = UE * (1 + 2 * background_v2 * std::cos(2 * (tower_phi - background_Psi2)));
      }
      float new_energy = raw_energy - UE;
      // if a tower is masked, leave it at zero
      if (!tower->get_isGood())
      {
        new_energy = 0;
      }

      ohcal_towerinfos->get_tower_at_channel(channel)->set_time(tower->get_time());
      ohcal_towerinfos->get_tower_at_channel(channel)->set_energy(new_energy);
      if (Verbosity() > 5)
      {
        std::cout << "SubtractTowers::process_event : OHCal tower at ieta / iphi = " << ieta << " / " << iphi << ", pre-sub / after-sub E = " << raw_energy << " / " << new_energy << std::endl;
      }
    }
  }

  if (Verbosity() > 0)
  {
    if (!m_use_towerinfo)
    {
      std::cout << "SubtractTowers::process_event: ending with " << emcal_towers->size() << " TOWER_CALIB_CEMC_RETOWER_SUB1 towers" << std::endl;
      std::cout << "SubtractTowers::process_event: ending with " << ihcal_towers->size() << " TOWER_CALIB_HCALIN_SUB1 towers" << std::endl;
      std::cout << "SubtractTowers::process_event: ending with " << ohcal_towers->size() << " TOWER_CALIB_HCALOUT_SUB1 towers" << std::endl;
    }
    else
    {
      std::cout << "SubtractTowers::process_event: ending with " << emcal_towerinfos->size() << m_towerNodePrefix << "_CEMC_RETOWER_SUB1 towers" << std::endl;
      std::cout << "SubtractTowers::process_event: ending with " << ihcal_towerinfos->size() << m_towerNodePrefix << "_HCALIN_SUB1 towers" << std::endl;
      std::cout << "SubtractTowers::process_event: ending with " << ohcal_towerinfos->size() << m_towerNodePrefix << "_HCALOUT_SUB1 towers" << std::endl;
    }
  }

  if (Verbosity() > 0)
  {
    std::cout << "SubtractTowers::process_event: exiting" << std::endl;
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

int SubtractTowers::CreateNode(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);

  // Looking for the DST node
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << PHWHERE << "DST Node missing, doing nothing." << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }

  IHTowerName = m_towerNodePrefix + "_HCALIN";
  TowerInfoContainer *hcal_towers = findNode::getClass<TowerInfoContainer>(topNode, IHTowerName);
  if (m_use_towerinfo && !hcal_towers)
  {
    std::cout << PHWHERE << "Cannot find " << IHTowerName << " for creating new tower containers. Exiting" << std::endl;
    exit(1);
  }

  // store the new EMCal towers

  PHCompositeNode *emcalNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "CEMC"));
  if (!emcalNode)
  {
    std::cout << PHWHERE << "EMCal Node note found, doing nothing." << std::endl;
  }
  if (m_use_towerinfo)
  {
    EMTowerName = m_towerNodePrefix + "_CEMC_RETOWER_SUB1";
    if(!m_use_retower) EMTowerName = m_towerNodePrefix + "_CEMC_SUB1";
    TowerInfoContainer *test_emcal_tower = findNode::getClass<TowerInfoContainer>(topNode, EMTowerName);
    if (!test_emcal_tower)
    {
      if (Verbosity() > 0)
      {
        std::cout << "SubtractTowers::CreateNode : creating " << EMTowerName << " node " << std::endl;
      }

      TowerInfoContainer *emcal_towers = dynamic_cast<TowerInfoContainer *>(hcal_towers->CloneMe());
      PHIODataNode<PHObject> *emcalTowerNode = new PHIODataNode<PHObject>(emcal_towers, EMTowerName, "PHObject");
      emcalNode->addNode(emcalTowerNode);
    }
    else
    {
      std::cout << "SubtractTowers::CreateNode : " << EMTowerName << " already exists! " << std::endl;
    }
  }


  // store the new IHCal towers
  PHCompositeNode *ihcalNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "HCALIN"));
  if (!ihcalNode)
  {
    std::cout << PHWHERE << "IHCal Node note found, doing nothing." << std::endl;
  }
  if (m_use_towerinfo)
  {
    IHTowerName = m_towerNodePrefix + "_HCALIN_SUB1";
    TowerInfoContainer *test_ihcal_tower = findNode::getClass<TowerInfoContainer>(topNode, IHTowerName);
    if (!test_ihcal_tower)
    {
      if (Verbosity() > 0)
      {
        std::cout << "SubtractTowers::CreateNode : creating " << IHTowerName << " node " << std::endl;
      }

      TowerInfoContainer *ihcal_towers = dynamic_cast<TowerInfoContainer *>(hcal_towers->CloneMe());
      PHIODataNode<PHObject> *ihcalTowerNode = new PHIODataNode<PHObject>(ihcal_towers, IHTowerName, "PHObject");
      ihcalNode->addNode(ihcalTowerNode);
    }
    else
    {
      std::cout << "SubtractTowers::CreateNode : " << IHTowerName << " already exists! " << std::endl;
    }
  }

  // store the new OHCal towers
  PHCompositeNode *ohcalNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "HCALOUT"));
  if (!ohcalNode)
  {
    std::cout << PHWHERE << "OHCal Node note found, doing nothing." << std::endl;
  }
  if (m_use_towerinfo)
  {
    OHTowerName = m_towerNodePrefix + "_HCALOUT_SUB1";
    TowerInfoContainer *test_ohcal_tower = findNode::getClass<TowerInfoContainer>(topNode, OHTowerName);
    if (!test_ohcal_tower)
    {
      if (Verbosity() > 0)
      {
        std::cout << "SubtractTowers::CreateNode : creating " << OHTowerName << " node " << std::endl;
      }

      TowerInfoContainer *ohcal_towers = dynamic_cast<TowerInfoContainer *>(hcal_towers->CloneMe());
      PHIODataNode<PHObject> *ohcalTowerNode = new PHIODataNode<PHObject>(ohcal_towers, OHTowerName, "PHObject");
      ohcalNode->addNode(ohcalTowerNode);
    }
    else
    {
      std::cout << "SubtractTowers::CreateNode : " << OHTowerName << " already exists! " << std::endl;
    }
  }

  return Fun4AllReturnCodes::EVENT_OK;
}
