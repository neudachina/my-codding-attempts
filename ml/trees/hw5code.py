import numpy as np
from collections import Counter


def find_best_split(feature_vector, target_vector):
    """
    Под критерием Джини здесь подразумевается следующая функция:
    $$Q(R) = -\frac {|R_l|}{|R|}H(R_l) -\frac {|R_r|}{|R|}H(R_r)$$,
    $R$ — множество объектов, $R_l$ и $R_r$ — объекты, попавшие в левое и правое поддерево,
     $H(R) = 1-p_1^2-p_0^2$, $p_1$, $p_0$ — доля объектов класса 1 и 0 соответственно.

    Указания:
    * Пороги, приводящие к попаданию в одно из поддеревьев пустого множества объектов, не рассматриваются.
    * В качестве порогов, нужно брать среднее двух сосдених (при сортировке) значений признака
    * Поведение функции в случае константного признака может быть любым.
    * При одинаковых приростах Джини нужно выбирать минимальный сплит.
    * За наличие в функции циклов балл будет снижен. Векторизуйте! :)

    :param feature_vector: вещественнозначный вектор значений признака
    :param target_vector: вектор классов объектов,  len(feature_vector) == len(target_vector)

    :return thresholds: отсортированный по возрастанию вектор со всеми возможными порогами, по которым объекты можно
     разделить на две различные подвыборки, или поддерева
    :return ginis: вектор со значениями критерия Джини для каждого из порогов в thresholds len(ginis) == len(thresholds)
    :return threshold_best: оптимальный порог (число)
    :return gini_best: оптимальное значение критерия Джини (число)
    """
    feature_vector = np.array(feature_vector)
    target_vector = np.array(target_vector)
    
    # сортируем наши вектора по значениям признака
    permutation = np.argsort(feature_vector)
    feature_vector = feature_vector[permutation]
    target_vector = target_vector[permutation]
    
    positive = target_vector.sum()                  # количество всех положительных объектов
    left_positive = np.cumsum(target_vector)[:-1]   # положительные в левой части
    right_positive = positive - left_positive       # положительные в правой части
    
    # реверсим наш вектор таргетов, чтобы аналогично посчитать про нули
    reversed_target_vector = 1 - target_vector     
    negative = reversed_target_vector.sum() 
    left_negative = np.cumsum(reversed_target_vector)[:-1]
    right_negative = negative - left_negative
    
    size = len(target_vector)
    left_sum = np.array(range(1, size))             # количество объектов по порогам слева
    right_sum = size - left_sum                     # количество справа 
    
    enthropy_left = 1 - np.square(left_positive/left_sum) - np.square(left_negative/left_sum)
    enthropy_right = 1 - np.square(right_positive/right_sum) - np.square(right_negative/right_sum)
    
    gini = -1. * (left_sum / size) * enthropy_left - (right_sum / size) * enthropy_right
    
    # все возможные пороги
    thresholds = (feature_vector[:-1] + feature_vector[1:]) / 2
    feature_vector = feature_vector[1:]
    mask = (thresholds != feature_vector)            # чекаем, чтобы порог не делил два одинаковых значения
    thresholds = thresholds[mask]               
    gini = gini[mask]
    
    # случай с константным признаком
    if len(gini) == 0:                      
        return None
    
    index = np.argmax(gini)
    return thresholds, gini, thresholds[index], gini[index]
    
    
    


class DecisionTree:
    def __init__(self, feature_types, max_depth=None, min_samples_split=None, min_samples_leaf=None):
        if np.any(list(map(lambda x: x != "real" and x != "categorical", feature_types))):
            raise ValueError("There is unknown feature type")

        self._tree = {}
        self._feature_types = feature_types
        self._max_depth = max_depth
        self._min_samples_split = min_samples_split
        self._min_samples_leaf = min_samples_leaf

    def _fit_node(self, sub_X, sub_y, node):
        if np.all(sub_y == sub_y[0]):
            node["type"] = "terminal"
            node["class"] = sub_y[0]
            return

        feature_best, threshold_best, gini_best, split = None, None, None, None
        for feature in range(sub_X.shape[1]):
            feature_type = self._feature_types[feature]
            categories_map = {}

            if feature_type == "real":
                feature_vector = sub_X[:, feature]
            elif feature_type == "categorical":
                counts = Counter(sub_X[:, feature])
                clicks = Counter(sub_X[sub_y == 1, feature])
                ratio = {}
                for key, current_count in counts.items():
                    current_click = clicks.get(key, 0)
                    ratio[key] = current_click / current_count
                sorted_categories = list(map(lambda x: x[0], sorted(ratio.items(), key=lambda x: x[1])))
                categories_map = dict(zip(sorted_categories, list(range(len(sorted_categories)))))

                feature_vector = np.array(list(map(lambda x: categories_map[x], sub_X[:, feature])))
            else:
                raise ValueError
            
            if len(np.unique(feature_vector)) == 1:
                continue

            _, _, threshold, gini = find_best_split(feature_vector, sub_y)
            if gini_best is None or gini > gini_best:
                feature_best = feature
                gini_best = gini
                split = feature_vector < threshold

                if feature_type == "real":
                    threshold_best = threshold
                elif feature_type == "categorical":
                    threshold_best = list(map(lambda x: x[0],
                                              filter(lambda x: x[1] < threshold, categories_map.items())))
                else:
                    raise ValueError

        if feature_best is None:
            node["type"] = "terminal"
            node["class"] = Counter(sub_y).most_common(1)[0][0]
            return

        node["type"] = "nonterminal"

        node["feature_split"] = feature_best
        if self._feature_types[feature_best] == "real":
            node["threshold"] = threshold_best
        elif self._feature_types[feature_best] == "categorical":
            node["categories_split"] = threshold_best
        else:
            raise ValueError
        node["left_child"], node["right_child"] = {}, {}
        self._fit_node(sub_X[split], sub_y[split], node["left_child"])
        self._fit_node(sub_X[np.logical_not(split)], sub_y[np.logical_not(split)], node["right_child"])

    def _predict_node(self, x, node):
        while node['type'] == "nonterminal":
            feature = node["feature_split"]
            if ((self._feature_types[feature] == "real" and node["threshold"] > x[feature]) 
                or (self._feature_types[feature] == "categorical" and x[feature] in node["categories_split"])):
                    node = node["left_child"]
            else:
                node = node["right_child"]
        return node['class']

    def fit(self, X, y):
        self._fit_node(X, y, self._tree)

    def predict(self, X):
        predicted = []
        for x in X:
            predicted.append(self._predict_node(x, self._tree))
        return np.array(predicted)

    def get_params(self, deep):
        return {'feature_types' : self._feature_types}
