from dataclasses import dataclass
from enum import auto
from enum import Enum
from typing import Dict
from typing import Type

import numpy as np
from numpy.random import sample


@dataclass
class LearningRate:
    lambda_: float = 1e-3
    s0: float = 1
    p: float = 0.5

    iteration: int = 0

    def __call__(self):
        """
        Calculate learning rate according to lambda (s0/(s0 + t))^p formula
        """
        self.iteration += 1
        return self.lambda_ * (self.s0 / (self.s0 + self.iteration)) ** self.p


class LossFunction(Enum):
    MSE = auto()
    MAE = auto()
    LogCosh = auto()
    Huber = auto()


class BaseDescent:
    """
    A base class and templates for all functions
    """

    def __init__(self, dimension: int, lambda_: float = 1e-3, delta: float = 1.0,
                 loss_function: LossFunction = LossFunction.MSE):
        """
        :param dimension: feature space dimension
        :param lambda_: learning rate parameter
        :param loss_function: optimized loss function
        """
        self.w: np.ndarray = np.random.rand(dimension)
        self.lr: LearningRate = LearningRate(lambda_=lambda_)
        self.loss_function: LossFunction = loss_function
        self.delta = delta

    def step(self, x: np.ndarray, y: np.ndarray) -> np.ndarray:
        return self.update_weights(self.calc_gradient(x, y))

    def update_weights(self, gradient: np.ndarray) -> np.ndarray:
        """
        Template for update_weights function
        Update weights with respect to gradient
        :param gradient: gradient
        :return: weight difference (w_{k + 1} - w_k): np.ndarray
        """
        pass

    def calc_gradient(self, x: np.ndarray, y: np.ndarray) -> np.ndarray:
        """
        Template for calc_gradient function
        Calculate gradient of loss function with respect to weights
        :param x: features array
        :param y: targets array
        :return: gradient: np.ndarray
        """
        pass

    def calc_loss(self, x: np.ndarray, y: np.ndarray) -> float:
        """
        Calculate loss for x and y with our weights
        :param x: features array
        :param y: targets array
        :return: loss: float
        """
        matrix = y - (x @ self.w)
        l = x.shape[0]
        return (matrix.T @ matrix) / l

    def predict(self, x: np.ndarray) -> np.ndarray:
        """
        Calculate predictions for x
        :param x: features array
        :return: prediction: np.ndarray
        """
        return (x @ self.w)


class VanillaGradientDescent(BaseDescent):
    """
    Full gradient descent class
    """

    def update_weights(self, gradient: np.ndarray) -> np.ndarray:
        """
        :return: weight difference (w_{k + 1} - w_k): np.ndarray
        """
        difference = -self.lr() * gradient
        self.w += difference
        return difference

    def calc_gradient(self, x: np.ndarray, y: np.ndarray) -> np.ndarray:
        if self.loss_function is LossFunction.MSE:
            matrix = self.predict(x) - y
            l = x.shape[0]
            return 2 / l * (x.T @ matrix)
        
        elif self.loss_function is LossFunction.LogCosh:
            return np.tanh(self.predict(x) - y) @ x / y.shape[0]
        
        elif self.loss_function is LossFunction.MAE:
            return x.T * np.sign(self.predict(x) - y) / y.shape[0]
        
        elif self.loss_function is LossFunction.Huber:
            abs_error = np.abs(y - self.predict(x))
            
            indexes = np.where(abs_error < self.delta)[0]
            less_x = x[indexes]
            less_y = y[indexes]
            
            indexes = np.where(abs_error >= self.delta)[0]
            more_x = x[indexes]
            more_y = y[indexes]
            
            return (less_x.T @ (self.predict(less_x) - less_y) 
                    + self.delta * (more_x.T @ np.sign(self.predict(more_x) - more_y))) / y.shape[0]
            
            


class StochasticDescent(VanillaGradientDescent):
    """
    Stochastic gradient descent class
    """

    def __init__(self, dimension: int, lambda_: float = 1e-3, batch_size: int = 50,
                 loss_function: LossFunction = LossFunction.MSE):
        """
        :param batch_size: batch size (int)
        """
        super().__init__(dimension, lambda_, loss_function)
        self.batch_size = batch_size

    def calc_gradient(self, x: np.ndarray, y: np.ndarray) -> np.ndarray:
        indexes = np.random.choice(x.shape[0], size=self.batch_size, replace=False)
        random_x = x[indexes]
        random_y = y[indexes]
        if self.loss_function is LossFunction.MSE:
            matrix = (random_x @ self.w) - random_y
            return 2 * (random_x.T @ matrix) / self.batch_size
        
        elif self.loss_function is LossFunction.LogCosh:
            return np.tanh(random_x @ self.w - random_y) @ random_x / self.batch_size
        
        elif self.loss_function is LossFunction.MAE:
            return random_x.T * np.sign(self.predict(random_x) - random_y) / self.batch_size
        
        elif self.loss_function is LossFunction.Huber:
            abs_error = np.abs(random_y - self.predict(random_x))
            
            indexes = np.where(abs_error < self.delta)[0]
            less_x = random_x[indexes]
            less_y = random_y[indexes]
            
            indexes = np.where(abs_error >= self.delta)[0]
            more_x = random_x[indexes]
            more_y = random_y[indexes]
            
            return (less_x.T @ (self.predict(less_x) - less_y) 
                    + self.delta * (more_x.T @ np.sign(self.predict(more_x) - more_y))) / self.batch_size


class MomentumDescent(VanillaGradientDescent):
    """
    Momentum gradient descent class
    """

    def __init__(self, dimension: int, lambda_: float = 1e-3, loss_function: LossFunction = LossFunction.MSE):
        super().__init__(dimension, lambda_, loss_function)
        self.alpha: float = 0.9

        self.h: np.ndarray = np.zeros(dimension)

    def update_weights(self, gradient: np.ndarray) -> np.ndarray:
        """
        :return: weight difference (w_{k + 1} - w_k): np.ndarray
        """
        self.h = self.alpha * self.h + self.lr() * gradient
        self.w -= self.h
        return -self.h


class Adam(VanillaGradientDescent):
    """
    Adaptive Moment Estimation gradient descent class
    """

    def __init__(self, dimension: int, lambda_: float = 1e-3, loss_function: LossFunction = LossFunction.MSE):
        super().__init__(dimension, lambda_, loss_function)
        self.eps: float = 1e-8

        self.m: np.ndarray = np.zeros(dimension)
        self.v: np.ndarray = np.zeros(dimension)

        self.beta_1: float = 0.9
        self.beta_2: float = 0.999

        self.iteration: int = 1

    def update_weights(self, gradient: np.ndarray) -> np.ndarray:
        """
        :return: weight difference (w_{k + 1} - w_k): np.ndarray
        """
        self.m = self.beta_1 * self.m + (1 - self.beta_1) * gradient
        self.v = self.beta_2 * self.v + (1 - self.beta_2) * np.square(gradient)
        
        self.iteration += 1
        m_cap = self.m / (1 - np.power(self.beta_1, self.iteration))
        v_cap = self.v / (1 - np.power(self.beta_2, self.iteration + 1))
        
        difference = -self.lr() * m_cap / (np.sqrt(v_cap) + self.eps)
        self.w += difference
        return difference
    
    
class AMSGrad(VanillaGradientDescent):
    def __init__(self, dimension: int, lambda_: float = 1e-3, loss_function: LossFunction = LossFunction.MSE):
        super().__init__(dimension, lambda_, loss_function)
        self.eps: float = 1e-8

        self.m: np.ndarray = np.zeros(dimension)
        self.v: np.ndarray = np.zeros(dimension)
        self.v_cap: np.ndarray = np.zeros(dimension)

        self.beta_1: float = 0.9
        self.beta_2: float = 0.999

        self.iteration: int = 1

    def update_weights(self, gradient: np.ndarray) -> np.ndarray:
        """
        :return: weight difference (w_{k + 1} - w_k): np.ndarray
        """
        self.m = self.beta_1 * self.m + (1 - self.beta_1) * gradient
        self.v = self.beta_2 * self.v + (1 - self.beta_2) * np.square(gradient)
            
        self.iteration += 1
        self.v_cap = np.maximum(self.v, self.v_cap)
            
        difference = -self.lr() * self.m / (np.sqrt(self.v) + self.eps)
        self.w += difference
        return difference    


class BaseDescentReg(BaseDescent):
    """
    A base class with regularization
    """

    def __init__(self, *args, mu: float = 0, **kwargs):
        """
        :param mu: regularization coefficient (float)
        """
        super().__init__(*args, **kwargs)

        self.mu = mu

    def calc_gradient(self, x: np.ndarray, y: np.ndarray) -> np.ndarray:
        """
        Calculate gradient of loss function and L2 regularization with respect to weights
        """
        weights = self.w
        weights[-1] = 0
        l2_gradient: np.ndarray = weights
        return super().calc_gradient(x, y) + l2_gradient * self.mu


class VanillaGradientDescentReg(BaseDescentReg, VanillaGradientDescent):
    """
    Full gradient descent with regularization class
    """


class StochasticDescentReg(BaseDescentReg, StochasticDescent):
    """
    Stochastic gradient descent with regularization class
    """


class MomentumDescentReg(BaseDescentReg, MomentumDescent):
    """
    Momentum gradient descent with regularization class
    """


class AdamReg(BaseDescentReg, Adam):
    """
    Adaptive gradient algorithm with regularization class
    """
    
class AMSGradReg(BaseDescentReg, AMSGrad):
    """
    Adaptive gradient algorithm with regularization class
    """


def get_descent(descent_config: dict) -> BaseDescent:
    descent_name = descent_config.get('descent_name', 'full')
    regularized = descent_config.get('regularized', False)

    descent_mapping: Dict[str, Type[BaseDescent]] = {
        'full': VanillaGradientDescent if not regularized else VanillaGradientDescentReg,
        'stochastic': StochasticDescent if not regularized else StochasticDescentReg,
        'momentum': MomentumDescent if not regularized else MomentumDescentReg,
        'adam': Adam if not regularized else AdamReg,
        'amsgrad': AMSGrad if not regularized else AMSGradReg
    }

    if descent_name not in descent_mapping:
        raise ValueError(f'Incorrect descent name, use one of these: {descent_mapping.keys()}')

    descent_class = descent_mapping[descent_name]

    return descent_class(**descent_config.get('kwargs', {}))
