# GuidedLinearUpsampling

The code of our paper [Guided Linear Upsampling, SIGGRAPH'2023](https://arxiv.org/abs/2307.09582), which provides a simple and effective way for high-resolution image processing.


#### Installation
The code is dependent on only OpenCV (3.x) and is tested with visual studio 2019, but it should be easy to be ported to other platforms such as Linux.

In visual studio you can modify the macros in config.props to config OpenCV on your machine.

#### Run the demo

We have provided a pair of test images so that you can test the code easily. In the main() function you can switch the commands for self upsampling or gudied upsampling.

#### Bibtex

@article{glu2023,
  title={Guided Linear Upsampling},
  author={Song, Shuangbing and Zhong, Fan and Wang, Tianju and Qin, Xueying and Tu, Changhe},
  journal={ACM Transactions on Graphics (Siggraph'2023)},
  year={2023},
}
