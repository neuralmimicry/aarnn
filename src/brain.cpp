#include "brain.h"

#include "axon.h"
#include "axon_bouton.h"
#include "axon_branch.h"
#include "axon_hillock.h"
#include "dendrite.h"
#include "dendrite_bouton.h"
#include "dendrite_branch.h"
#include "effector.h"
#include "neuron.h"
#include "position.h"
#include "sensory_receptor.h"
#include "soma.h"
#include "synaptic_gap.h"

namespace ns_aarnn
{
void Brain::initialise(size_t numberOfNeurons,
                       size_t numberOfVisualInputs,
                       size_t numberOfPixels,
                       size_t numberOfAuditoryInputs,
                       size_t numberOfPhonels,
                       size_t numberOfOlfactoryInputs,
                       size_t numberOfScentels,
                       size_t numberOfVocalEffectors,
                       size_t numberOfVocels)
{
    createNeurons_(numberOfNeurons);
    createVisualReceptors_(numberOfVisualInputs, numberOfPixels);
    createAuditoryReceptors_(numberOfAuditoryInputs, numberOfPhonels);
    createOlfactoryReceptors_(numberOfOlfactoryInputs, numberOfScentels);
    createVocalEffectors_(numberOfVocalEffectors, numberOfVocels);
    performSynapticAssociation_();
}

void Brain::createNeurons_(size_t numberOfNeurons)
{
    // Create a list of neurons
    spdlog::info("Creating neurons...");
    std::mutex neuron_mutex;
    std::mutex empty_neuron_mutex;
    neurons_.clear();

    auto prevNeuron = neurons_.back();
    // Currently the application places neurons in a defined pattern but the pattern is not based on cell growth
    for(size_t i = 0; i < numberOfNeurons; ++i)
    {
        auto d_i    = static_cast<double>(i);
        auto coords = Position::layeredFibonacciSpherePoint(i, numberOfNeurons);
        if(i > 0)
        {
            prevNeuron = neurons_.back();
            coords.move((0.1 * d_i) + sin((3.14 / 180) * (d_i * 10)) * 0.1,
                        (0.1 * d_i) + cos((3.14 / 180) * (d_i * 10)) * 0.1,
                        (0.1 * d_i) + sin((3.14 / 180) * (d_i * 10)) * 0.1);
        }

        neurons_.emplace_back(Neuron::create(coords));
        neurons_.back()->initialise();
        // Sparsely associate neurons
        if(i > 0 && i % 3 == 0)
        {
            auto prevDendriteBouton =
             prevNeuron->getChild<Soma>()->getDendriteBranches()[0]->getDendrites()[0]->getChild<DendriteBouton>();
            neurons_.back()
             ->getChild<Soma>()
             ->getAxonHillock()
             ->getChild<Axon>()
             ->getChild<AxonBouton>()
             ->getChild<SynapticGap>()
             ->moveRelativeTo(*prevDendriteBouton, 0.4, 0.4, 0.4);

            neurons_.back()
             ->getChild<Soma>()
             ->getAxonHillock()
             ->getChild<Axon>()
             ->getChild<AxonBouton>()
             ->moveRelativeTo(*prevDendriteBouton, 0.8, 0.8, 0.8);

            neurons_.back()->getChild<Soma>()->getAxonHillock()->getChild<Axon>()->scale(0.5).moveRelativeTo(
             *prevDendriteBouton,
             1.2,
             1.2,
             1.2);

            // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
            neurons_[i - 1]->associateSynapticGap(neurons_[i], proximityThreshold_);
        }
    }
    spdlog::info("Created {} neurons.", neurons_.size());
}

void Brain::createVisualReceptors_(size_t numberOfVisualInputs, size_t numberOfPixels)
{
    // Create a collection of visual inputs (eyes)
    spdlog::info("Creating {} visual sensory inputs...", numberOfVisualInputs);

    visualInputs_.clear();
    visualInputs_.resize(numberOfVisualInputs);

    auto prevReceptor = visualInputs_.rbegin();
#pragma omp parallel for
    for(size_t j = 0; j < numberOfVisualInputs; ++j)
    {
        auto d_j = static_cast<double>(j);
        for(size_t i = 0; i < (numberOfPixels / numberOfVisualInputs); ++i)
        {
            auto coords = Position::layeredFibonacciSpherePoint(i, numberOfPixels);
            if(i > 0)
            {
                prevReceptor = visualInputs_.rbegin();
                coords.move(-100.0 + (d_j * 200.0), 0.0, -100.0);
            }

            visualInputs_[j].emplace_back(SensoryReceptor::create(coords));
            visualInputs_[j].back()->initialise();

            // Sparsely associate neurons
            if(i > 0 && i % 7 == 0)
            {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
                // components too

                neurons_[int(i + ((numberOfPixels / 2) * j))]
                 ->getChild<Soma>()
                 ->getDendriteBranches()[0]
                 ->getDendrites()[0]
                 ->getChild<DendriteBouton>()
                 ->moveRelativeTo(*(visualInputs_[j].back()->getSynapticGaps()[0]), 0.4, 0.4, 0.4);

                neurons_[int(i + ((numberOfPixels / 2) * j))]
                 ->getChild<Soma>()
                 ->getDendriteBranches()[0]
                 ->getDendrites()[0]
                 ->moveRelativeTo(*(visualInputs_[j].back()->getSynapticGaps()[0]), 0.8, 0.8, 0.8);
                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                visualInputs_[j].back()->associateSynapticGap(neurons_[int(i + ((numberOfPixels / 2) * j))],
                                                              proximityThreshold_);
            }
        }
    }
    spdlog::info("Created {} sensory receptors.", (visualInputs_[0].size() + visualInputs_[1].size()));
}

void Brain::createAuditoryReceptors_(size_t numberOfAuditoryInputs, size_t numberOfPhonels)
{
    // create a collection of audio inputs (pair of ears)
    spdlog::info("Creating auditory sensory inputs...");

    audioInputs_.clear();
    audioInputs_.resize(numberOfAuditoryInputs);

    auto prevReceptor = audioInputs_.rbegin();

#pragma omp parallel for
    for(size_t j = 0; j < 2; ++j)
    {
        auto d_j = static_cast<double>(j);
        for(size_t i = 0; i < (numberOfPhonels / numberOfAuditoryInputs); ++i)
        {
            auto coords = Position::layeredFibonacciSpherePoint(i, numberOfPhonels);
            if(i > 0)
            {
                prevReceptor = audioInputs_.rbegin();
                coords.move(-150.0 + (d_j * 200.0), 0.0, -100.0);
            }

            audioInputs_[j].emplace_back(SensoryReceptor::create(coords));
            audioInputs_[j].back()->initialise();
            // Sparsely associate neurons
            if(i > 0 && i % 11 == 0)
            {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
                // components
                neurons_[int(i + ((numberOfPhonels / 2) * j))]
                 ->getChild<Soma>()
                 ->getDendriteBranches()[0]
                 ->getDendrites()[0]
                 ->getChild<DendriteBouton>()
                 ->moveRelativeTo(*(audioInputs_[j].back()->getSynapticGaps()[0]), 0.4, 0.4, 0.4);
                neurons_[int(i + ((numberOfPhonels / 2) * j))]
                 ->getChild<Soma>()
                 ->getDendriteBranches()[0]
                 ->getDendrites()[0]
                 ->moveRelativeTo(*(audioInputs_[j].back()->getSynapticGaps()[0]), 0.8, 0.8, 0.8);
                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                audioInputs_[j].back()->associateSynapticGap(neurons_[int(i + ((numberOfPhonels / 2) * j))],
                                                             proximityThreshold_);
            }
        }
    }
    spdlog::info("Created {} sensory receptors.", (audioInputs_[0].size() + audioInputs_[1].size()));
}

void Brain::createOlfactoryReceptors_(size_t numberOfOlfactoryInputs, size_t numberOfScentels)
{
    // Create a collection of olfactory inputs (pair of nostrils)
    spdlog::info("Creating olfactory sensory inputs...");

    // Resize the vector to contain 2 elements
    olfactoryInputs_.clear();
    olfactoryInputs_.resize(numberOfOlfactoryInputs);

    auto prevReceptor = olfactoryInputs_[0].rbegin();

#pragma omp parallel for
    for(size_t j = 0; j < 2; ++j)
    {
        auto d_j = static_cast<double>(j);
        for(size_t i = 0; i < (numberOfScentels / 2); ++i)
        {
            auto coords = Position::layeredFibonacciSpherePoint(i, numberOfScentels);
            if(i > 0)
            {
                prevReceptor = olfactoryInputs_[j].rbegin();
                coords.move(-20.0 + (d_j * 40.0), -10.0, -10.0);
            }

            olfactoryInputs_[j].emplace_back(SensoryReceptor::create(coords));
            olfactoryInputs_[j].back()->initialise();
            // Sparsely associate neurons
            if(i > 0 && i % 13 == 0)
            {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
                // components too
                neurons_[int(i + ((numberOfScentels / 2) * j))]
                 ->getChild<Soma>()
                 ->getDendriteBranches()[0]
                 ->getDendrites()[0]
                 ->getChild<DendriteBouton>()
                 ->moveRelativeTo(*(olfactoryInputs_[j].back()->getSynapticGaps()[0]), 0.4, 0.4, 0.4);

                neurons_[int(i + ((numberOfScentels / 2) * j))]
                 ->getChild<Soma>()
                 ->getDendriteBranches()[0]
                 ->getDendrites()[0]
                 ->moveRelativeTo(*(olfactoryInputs_[j].back()->getSynapticGaps()[0]), 0.8, 0.8, 0.8);

                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                olfactoryInputs_[j].back()->associateSynapticGap(neurons_[int(i + ((numberOfScentels / 2) * j))],
                                                                 proximityThreshold_);
            }
        }
    }
    spdlog::info("Created {} sensory receptors.", (olfactoryInputs_[0].size() + olfactoryInputs_[1].size()));
}

void Brain::createVocalEffectors_(size_t numberOfVocalEffectors, size_t numberOfVocels)
{
    // Create a collection of vocal effector outputs (vocal tract cords/muscle control)
    spdlog::info("Creating vocal effector outputs...");
    auto prevEffector = vocalOutputs_.rbegin();

    for(size_t i = 0; i < (numberOfVocels); ++i)
    {
        auto coords = Position::layeredFibonacciSpherePoint(i, numberOfVocels);
        if(i > 0)
        {
            prevEffector = vocalOutputs_.rbegin();
            coords.move(0.0, -100.0, 10.0);
        }

        vocalOutputs_.emplace_back(Effector::create(coords));
        vocalOutputs_.back()->initialise();
        // Sparsely associate neurons
        if(i > 0 && i % 17 == 0
           && !neurons_[int(i + numberOfVocels)]
                ->getChild<Soma>()
                ->getAxonHillock()
                ->getChild<Axon>()
                ->getChild<AxonBouton>()
                ->getChild<SynapticGap>()
                ->isAssociated())
        {
            // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
            // components too
            neurons_[int(i + numberOfVocels)]
             ->getChild<Soma>()
             ->getAxonHillock()
             ->getChild<Axon>()
             ->getChild<AxonBouton>()
             ->getChild<SynapticGap>()
             ->moveRelativeTo(*vocalOutputs_.back(), -0.4, -0.4, -0.4);

            neurons_[int(i + numberOfVocels)]->getChild<Soma>()->getAxonHillock()->getChild<Axon>()->moveRelativeTo(
             *(neurons_[int(i + numberOfVocels)]
                ->getChild<Soma>()
                ->getAxonHillock()
                ->getChild<Axon>()
                ->getChild<AxonBouton>()),
             0.8,
             0.8,
             0.8);

            // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
            neurons_[int(i + numberOfVocels)]
             ->getChild<Soma>()
             ->getAxonHillock()
             ->getChild<Axon>()
             ->getChild<AxonBouton>()
             ->getChild<SynapticGap>()
             ->setAsAssociated();
        }
    }
    spdlog::info("Created {} effectors.", vocalOutputs_.size());
}

void Brain::performSynapticAssociation_()
{
// Assuming neurons is a std::vector<Neuron*> or similar container
// and associateSynapticGap is a function you've defined elsewhere

// Parallelize synaptic association
#pragma omp parallel for schedule(dynamic)
    for(size_t i = 0; i < neurons_.size(); ++i)
    {
        for(size_t j = i + 1; j < neurons_.size(); ++j)
        {
            neurons_[i]->associateSynapticGap(neurons_[j], proximityThreshold_);
        }
    }

    // Use a fixed number of threads_
    const size_t numberOfThreads = std::thread::hardware_concurrency();

    threads_.reserve(numberOfThreads);

    size_t neuronsPerThread = neurons_.size() / numberOfThreads;

    auto& neurons = neurons_;
    // Calculate the propagation rate in parallel
    for(size_t t = 0; t < numberOfThreads; ++t)
    {
        size_t start = t * neuronsPerThread;
        size_t end   = (t + 1) * neuronsPerThread;
        if(t == numberOfThreads - 1)
            end = neurons_.size();  // Handle remainder

        threads_.emplace_back(
         [=, &neurons]()
         {
             for(size_t i = start; i < end; ++i)
             {
                 neurons[i]->computePropagationRate(totalPropagationRate_);
             }
         });
    }

    // Join the threads_
    for(std::thread& t: threads_)
    {
        t.join();
    }

    // Get the total propagation rate
    double propagationRate = totalPropagationRate_.load();

    spdlog::info("The propagation rate is {}", propagationRate);
}

}  // namespace aarnn
