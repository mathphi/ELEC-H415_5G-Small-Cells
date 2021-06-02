# Channel Modeling for 5G Small Cells
#### ELEC-H415 - Communication channels

![5G Small Cells - Capture](5G%20Small%20Cells%20-%20Simulation%20-%20Capture.jpg "5G Small Cells - Capture")

### Objectives
5G small cells will be soon deployed to provide very high data rates, at street level. They will complement the usual urban cells whose base stations are located on rooftops, typically. Small cell base stations (BS) will be installed on urban furniture, at height comparable to user equipment (UE). They will communicate at frequencies around 27GHz (in Europe), with very large bandwidths, up to 200MHz in Belgium. The goal of this project is to numerically model small cell channels, by using ray-tracing to simulate propagation.

### Scenario
You are asked to model the downlink channel from a small cell BS towards a mobile UE, under the following assumptions.

##### Communication parameters
- Carrier frequency: 27GHz.
- BS and UE at the same height (2m).
- Transmit and receive antennas are vertical half-wave dipoles.
- Maximum EIRP: 2W (deduce transmit power as a function of used antenna).
- Target SNR at UE: 2dB.
- Receiver noise figure at UE: 10dB.

##### Geometry
Channel will be simulated in a geometry representative of Rue de la Loi/Wetstraat in Brussels.

##### Ray-tracing parameters
- Ground and buildings have relative permittivities equal to 5.
- The image theory is used to calculate the propagation paths.
- In the street of the BS, following rays must be calculated: Line-of-Sight (LOS), single, double and triple reflections off buildings, reflection off the ground.
- In the streets crossing the BS street, if LOS exists: LOS and reflection off the ground are taken into account. If NLOS: only diffraction is taken into account.
- Calculation starts at a minimal distance of 10m to the BS.
- The street area is divided into local areas of 1 m 2 . All local parameters (power, SNR...) should be calculated at the center of each local area only.
- Due to the simple scenario, the propagation model you will deduce won’t be based on the usual three scales of analysis (path loss, shadowing, small scale fading) but on two scales only: path loss and a global fading around path loss and assumed to be log-normally distributed (which is only approximate in this calculation). That means that you have first to deduce a path loss model from your calculation, and, then, any power variation around this path loss is considered as a log-normal fading whose variability is to be estimated.

### Minimal requirements
All channel parameters seen in the course can be estimated by using this approach.

By placing a BS at the red dot on the previous figure, following results must at least be given as (i) a 1D plot along Rue de la Loi/Wetstraat, and (ii) a heat map over the main and crossing streets:
- Received power (mandatory to plot the 1D graph in your report, for validation purpose of your code).
- SNR at UE.
- Delay spread.
- Rice factor.

At the center of following three intersections: Rue du Commerce/Handelsstraat, Rue de la Science/Wetenschapsstraat, Rue de Trèves/Trierstraat:
- Ray paths between the BS and UE.
- Physical impulse response.
- TDL impulse response (for different bandwidths, from narrowband till 200 MHz, at least).
- Uncorrelated scattering TDL impulse response.

From your simulations, a propagation model suitable for small cell deployment must be built:
- Path loss model
- Fading variability
- Cell range as a function of connection probability at cell edge. Probability of coverage through the whole cell can be deduced.
- Approximate penetration depth of coverage into crossing streets. There is no definite value for that, you have to define your own criteria.

Discuss obtained results.

### Challenge

It is easy to go beyond these minimal requirements, and you are invited to be creative!

We propose you to address the following challenge: design a BS deployment in the area between Rue de la Loi/Wetstraat and Rue Belliard/Belliardstraat, as shown on the map below. The number of BSs should be as small as possible while ensuring the highest probability of connection.

Other improvements can be easily found, as taking beamforming into account.
