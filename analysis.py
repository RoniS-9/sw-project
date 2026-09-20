"""Compare SymNMF and K-means clustering.

Usage: python3 analysis.py k file_name.txt
Prints the silhouette score of each clustering and the adjusted Rand index
between them, each with 4 decimal places.
"""
import sys

import numpy as np
from sklearn.metrics import silhouette_score, adjusted_rand_score

import symnmf

EPSILON = 1e-4
MAX_ITER = 300


def euclidean_distance(point1, point2):
    """Euclidean distance between two points given as lists of floats."""
    sum_sq = 0.0
    for i in range(len(point1)):
        sum_sq += (point1[i] - point2[i]) ** 2
    return sum_sq ** 0.5


def closest_centroid(point, centroids):
    """Index of the centroid closest to point (ties -> lowest index)."""
    min_dist = float('inf')
    cluster_id = 0
    for i in range(len(centroids)):
        dist = euclidean_distance(point, centroids[i])
        if dist < min_dist:
            min_dist = dist
            cluster_id = i
    return cluster_id


def assign_labels(points, centroids):
    """Label of every point = index of its closest centroid."""
    return [closest_centroid(point, centroids) for point in points]


def update_centroids(points, labels, centroids):
    """New centroids = mean of the points assigned to each cluster.
    An empty cluster keeps its previous centroid."""
    d = len(points[0])
    new_centroids = []
    for i in range(len(centroids)):
        cluster_points = [p for p, lab in zip(points, labels) if lab == i]
        if len(cluster_points) == 0:
            new_centroids.append(list(centroids[i]))
            continue
        new_centroid = []
        for j in range(d):
            sum_xi = 0.0
            for p in cluster_points:
                sum_xi += p[j]
            new_centroid.append(sum_xi / len(cluster_points))
        new_centroids.append(new_centroid)
    return new_centroids


def max_shift(old, new):
    """Largest Euclidean distance between matching old and new centroids."""
    return max(euclidean_distance(o, n) for o, n in zip(old, new))


def kmeans(points, k):
    """HW1 K-means: first k points as initial centroids; iterate until every
    centroid moves less than EPSILON or MAX_ITER iterations were done.
    Returns the label of each point relative to the final centroids."""
    centroids = [list(points[i]) for i in range(k)]
    iteration_number = 0
    delta = EPSILON + 1.0
    while delta >= EPSILON and iteration_number < MAX_ITER:
        labels = assign_labels(points, centroids)
        new_centroids = update_centroids(points, labels, centroids)
        delta = max_shift(centroids, new_centroids)
        centroids = new_centroids
        iteration_number += 1
    return assign_labels(points, centroids)


def symnmf_labels(X, k):
    """SymNMF hard clustering: label of point i = argmax of row i of H."""
    H = symnmf.run_symnmf(X, k)
    return np.argmax(np.array(H), axis=1).tolist()


def parse_args(argv):
    """Validate the command line (as in symnmf.py) and return (k, X)."""
    if len(argv) != 3:
        symnmf.fail()
    k = symnmf.parse_k(argv[1])
    X = symnmf.load_points(argv[2])
    symnmf.check_k(k, len(X))
    return k, X


def main():
    """Entry point: cluster with both methods and print the scores."""
    np.random.seed(1234)
    k, X = parse_args(sys.argv)
    try:
        nmf_labels = symnmf_labels(X, k)
        km_labels = kmeans(X, k)
        nmf_score = silhouette_score(X, nmf_labels)
        km_score = silhouette_score(X, km_labels)
        ari = adjusted_rand_score(nmf_labels, km_labels)
    except Exception:
        symnmf.fail()
    print("nmf: %.4f" % nmf_score)
    print("kmeans: %.4f" % km_score)
    print("ari: %.4f" % ari)


if __name__ == "__main__":
    main()
