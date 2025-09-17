#include "SuperTowerReco.h"

#include <calobase/RawTower.h>
#include <calobase/RawTowerContainer.h>
#include <calobase/RawTowerDefs.h>
#include <calobase/RawTowerGeom.h>
#include <calobase/RawTowerGeomContainer.h>
#include <calobase/RawTowerv1.h>

#include <calobase/TowerInfo.h>
#include <calobase/TowerInfoContainer.h>
#include <calobase/TowerInfoDefs.h>

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
#include <cstdlib>
#include <iostream>
#include <utility>

SuperTowerReco::SuperTowerReco(const std::string &name)
  : SubsysReco(name)
{
}

int SuperTowerReco::InitRun(PHCompositeNode *topNode)
{
  CreateNode(topNode);
  return Fun4AllReturnCodes::EVENT_OK;
}

int SuperTowerReco::process_event(PHCompositeNode *topNode)
{
  if (Verbosity() > 0)
  {
    std::cout << "SuperTowerReco::process_event: entering" << std::endl;
  }

  
  //grab the retowered EMCal, IHCal, OHCal tower containers
  TowerInfoContainer *towerinfosEM3 = nullptr;
  TowerInfoContainer *towerinfosIH3 = nullptr;
  TowerInfoContainer *towerinfosOH3 = nullptr;
  EMTowerName = m_towerNodePrefix + "_CEMC_RETOWER";
  towerinfosEM3 = findNode::getClass<TowerInfoContainer>(topNode, EMTowerName);
  if (!towerinfosEM3)
    {
      std::cout << "SuperTowerReco::process_event: Cannot find node " << EMTowerName << std::endl;
      exit(1);
    }
  IHTowerName = m_towerNodePrefix + "_HCALIN";
  towerinfosIH3 = findNode::getClass<TowerInfoContainer>(topNode, IHTowerName);
  if (!towerinfosIH3)
    {
      std::cout << "SuperTowerReco::process_event: Cannot find node " << IHTowerName << std::endl;
      exit(1);
    }
  OHTowerName = m_towerNodePrefix + "_HCALOUT";
  towerinfosOH3 = findNode::getClass<TowerInfoContainer>(topNode, OHTowerName);
  if (!towerinfosOH3)
    {
      std::cout << "SuperTowerReco::process_event: Cannot find node " << OHTowerName << std::endl;
      exit(1);
    }

  //loop through each tower and sum up the energies
  unsigned int nchannels = towerinfosEM3->size();
  for (unsigned int channel = 0; channel < nchannels; channel++)
    {
      TowerInfo *tower = towerinfosEM3->get_tower_at_channel(channel);
      unsigned int channelkey = towerinfosEM3->encode_key(channel);
      int ieta = towerinfosEM3->getTowerEtaBin(channelkey);
      int iphi = towerinfosEM3->getTowerPhiBin(channelkey);
      rawtower_e[ieta][iphi] = tower->get_energy();
      rawtower_time[ieta][iphi] = tower->get_time_float() * tower->get_energy();
      rawtower_status[ieta][iphi] = tower->get_isGood();
    }
  for (unsigned int channel = 0; channel < nchannels; channel++)
    {
      TowerInfo *tower = towerinfosIH3->get_tower_at_channel(channel);
      unsigned int channelkey = towerinfosIH3->encode_key(channel);
      int ieta = towerinfosIH3->getTowerEtaBin(channelkey);
      int iphi = towerinfosIH3->getTowerPhiBin(channelkey);
      rawtower_e[ieta][iphi] += tower->get_energy();
      rawtower_time[ieta][iphi] += tower->get_time_float() * tower->get_energy();
      if(rawtower_status[ieta][iphi] == true) rawtower_status[ieta][iphi] = tower->get_isGood();
    }
  for (unsigned int channel = 0; channel < nchannels; channel++)
    {
      TowerInfo *tower = towerinfosOH3->get_tower_at_channel(channel);
      unsigned int channelkey = towerinfosOH3->encode_key(channel);
      int ieta = towerinfosOH3->getTowerEtaBin(channelkey);
      int iphi = towerinfosOH3->getTowerPhiBin(channelkey);
      rawtower_e[ieta][iphi] += tower->get_energy();
      rawtower_time[ieta][iphi] += tower->get_time_float() * tower->get_energy();
      if(rawtower_status[ieta][iphi] == true) rawtower_status[ieta][iphi] = tower->get_isGood();
    }
  
  SuperTowerName = m_towerNodePrefix + "_SUPER";
  TowerInfoContainer *super_tower = findNode::getClass<TowerInfoContainer>(topNode, SuperTowerName);
  if (Verbosity() > 0)
    {
      std::cout << "SuperTowerReco::process_event: filling " << SuperTowerName << " node" << std::endl;
    }
  for (int ieta_ihcal = 0; ieta_ihcal < neta_ihcal; ++ieta_ihcal)
    {
      for (int iphi_ihcal = 0; iphi_ihcal < nphi_ihcal; ++iphi_ihcal)
	{
	  unsigned int towerkey = TowerInfoDefs::encode_hcal(ieta_ihcal, iphi_ihcal);
	  unsigned int towerindex = super_tower->decode_key(towerkey);
	  TowerInfo *towerinfo = super_tower->get_tower_at_channel(towerindex);
	  towerinfo->set_energy(rawtower_e[ieta_ihcal][iphi_ihcal]);
	  if (rawtower_e[ieta_ihcal][iphi_ihcal] == 0)
	    {
	      towerinfo->set_time_float(0);
	    }
	  else
	    {
	      towerinfo->set_time_float((rawtower_time[ieta_ihcal][iphi_ihcal] / rawtower_e[ieta_ihcal][iphi_ihcal]));
	    }
	}
    }
  if (Verbosity() > 0)
    {
      std::cout << "SuperTowerReco::process_event: exiting" << std::endl;
    }
  return Fun4AllReturnCodes::EVENT_OK;
}

int SuperTowerReco::CreateNode(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << PHWHERE << "DST Node missing, doing nothing." << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }
  PHCompositeNode *emcalNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "CEMC"));
  if (!emcalNode)
  {
    std::cout << PHWHERE << "EMCal Node node found, doing nothing." << std::endl;
  }

  SuperTowerName = m_towerNodePrefix + "_SUPER";
  IHTowerName = m_towerNodePrefix + "_HCALIN";
  TowerInfoContainer *test_super_tower = findNode::getClass<TowerInfoContainer>(topNode, SuperTowerName);
  TowerInfoContainer *hcal_towers = findNode::getClass<TowerInfoContainer>(topNode, IHTowerName);
  if (!test_super_tower)
    {
      if (Verbosity() > 0)
	{
	  std::cout << "SuperTowerReco::CreateNode : creating " << SuperTowerName << " node " << std::endl;
	}
      if (!hcal_towers)
	{
	  std::cout << PHWHERE << " Could not find input HCAL tower node: " << IHTowerName << std::endl;
        exit(1);
	}
      TowerInfoContainer *super_tower = dynamic_cast<TowerInfoContainer *>(hcal_towers->CloneMe());
      PHIODataNode<PHObject> *superTowerNode = new PHIODataNode<PHObject>(super_tower, SuperTowerName, "PHObject");
      emcalNode->addNode(superTowerNode);
    }
  else
    {
      if (Verbosity() > 0)
	{
        std::cout << "SuperTowerReco::CreateNode : " << SuperTowerName << " already exists! " << std::endl;
	}
    }
  
  return Fun4AllReturnCodes::EVENT_OK;
}
