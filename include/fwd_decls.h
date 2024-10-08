#ifndef FWD_DECLS_H
#define FWD_DECLS_H

#include <memory>
#include <deque>

namespace ns_aarnn
{
class Axon;
using AxonPtr = std::shared_ptr<Axon>;
using Axons   = std::deque<AxonPtr>;

class AxonBouton;
using AxonBoutonPtr = std::shared_ptr<AxonBouton>;
using AxonBoutons   = std::deque<AxonBoutonPtr>;

class AxonBranch;
using AxonBranchPtr = std::shared_ptr<AxonBranch>;
using AxonBranches  = std::deque<AxonBranchPtr>;

class AxonHillock;
using AxonHillockPtr = std::shared_ptr<AxonHillock>;
using AxonHillocks   = std::deque<AxonHillockPtr>;

class BodyComponent;
using BodyComponentPtr = std::shared_ptr<BodyComponent>;
using BodyComponents   = std::deque<BodyComponentPtr>;

class Dendrite;
using DendritePtr = std::shared_ptr<Dendrite>;
using Dendrites   = std::deque<DendritePtr>;

class DendriteBranch;
using DendriteBranchPtr = std::shared_ptr<DendriteBranch>;
using DendriteBranches  = std::deque<DendriteBranchPtr>;

class DendriteBouton;
using DendriteBoutonPtr = std::shared_ptr<DendriteBouton>;
using DendriteBoutons   = std::deque<DendriteBoutonPtr>;

class Effector;
using EffectorPtr = std::shared_ptr<Effector>;
using Effectors   = std::deque<EffectorPtr>;

class Neuron;
using NeuronPtr     = std::shared_ptr<Neuron>;
using NeuronWeakPtr = std::weak_ptr<Neuron>;
using Neurons       = std::deque<NeuronPtr>;

class SensoryReceptor;
using SensoryReceptorPtr = std::shared_ptr<SensoryReceptor>;
using SensoryReceptors   = std::deque<SensoryReceptorPtr>;

class Soma;
using SomaPtr = std::shared_ptr<Soma>;
using Somas   = std::deque<SomaPtr>;

class SynapticGap;
using SynapticGapPtr = std::shared_ptr<SynapticGap>;
using SynapticGaps   = std::deque<SynapticGapPtr>;
}  // namespace aarnn

#endif  // FWD_DECLS_H
