#ifndef NEURON_PARAMETERS_H
#define NEURON_PARAMETERS_H

class NeuronParameters
{
public:
    NeuronParameters();

    /**
     * @brief Getter function for dendrite propagation rate.
     * @return The dendrite propagation rate.
     */
    double getDendritePropagationRate() const;

private:
    /**
     * @brief Neuron parameter: dendrite propagation rate.
     */
    double dendritePropagationRate;
};

#endif // NEURON_PARAMETERS_H
