import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


# Load data
def load_data(path):
    d = pd.read_csv(path)
    d['avg'] = (d['upper'] + d['lower']) / 2
    d['dura_norm'] = d['duration'] / abs(d['miss'] + d['hit'])
    d['dura_inv'] = abs(d['miss'] + d['hit']) / d['duration']
    return d


_map = load_data('./map_dump.csv')
_bst = load_data('./bst_dump.csv')
map_insert = _map[_map['action'] == 0]
map_erase = _map[_map['action'] == 3]
map_find = _map[_map['action'] == 1]
bst_insert = _bst[_bst['action'] == 0]
bst_erase = _bst[_bst['action'] == 3]
bst_find = _bst[_bst['action'] == 1]

# Fig
fig, ax = plt.subplots(nrows=1, ncols=1)
ax.set_xlabel('samples [ul]')
ax.set_ylabel('operation duration [s]')
ax.plot(map_insert['avg'], map_insert['dura_norm'], label='map<>.insert()')
ax.plot(map_erase['avg'], map_erase['dura_norm'], label='map<>.erase()')
ax.plot(map_find['avg'], map_find['dura_norm'], label='map<>.find()')
ax.plot(bst_insert['avg'], bst_insert['dura_norm'], label='bst<>.insert()')
ax.plot(bst_erase['avg'], bst_erase['dura_norm'], label='bst<>.erase()')
ax.plot(bst_find['avg'], bst_find['dura_norm'], label='bst<>.find()')
plt.legend()
fig.savefig('./duration.png')
plt.close(fig)

fig, ax = plt.subplots(nrows=1, ncols=1)
ax.set_xlabel('samples [ul]')
ax.set_ylabel('speed [kOPS]')

_mi = min(map_insert['avg'])
_mx = max(map_insert['avg'])
_mi = _mi * 0.8 + _mx * 0.2

ax.plot(map_insert[map_insert['avg'] > _mi]['avg'],
        map_insert[map_insert['avg'] > _mi]['dura_inv'] / 1e3, label='map<>.insert()')
ax.plot(map_erase[map_erase['avg'] > _mi]['avg'],
        map_erase[map_erase['avg'] > _mi]['dura_inv'] / 1e3, label='map<>.erase()')
ax.plot(map_find[map_find['avg'] > _mi]['avg'],
        map_find[map_find['avg'] > _mi]['dura_inv'] / 1e3, label='map<>.find()')
ax.plot(bst_insert[bst_insert['avg'] > _mi]['avg'],
        bst_insert[bst_insert['avg'] > _mi]['dura_inv'] / 1e3, label='bst<>.insert()')
ax.plot(bst_erase[bst_erase['avg'] > _mi]['avg'],
        bst_erase[bst_erase['avg'] > _mi]['dura_inv'] / 1e3, label='bst<>.erase()')
ax.plot(bst_find[bst_find['avg'] > _mi]['avg'],
        bst_find[bst_find['avg'] > _mi]['dura_inv'] / 1e3, label='bst<>.find()')
# ax.set_xlim(1e6)
# ax.set_ylim(0, 15e1)
# xmin, xmax = ax.get_xlim()
# ax.set_xlim(xmin * 0.8 + xmax * 0.2 , xmax)
plt.legend()
fig.savefig('./ops.png')
plt.close(fig)

fig, ax = plt.subplots(nrows=1, ncols=1)
ax.set_xlabel('samples [ul]')
ax.set_ylabel('normalized size of the tree [bytes]')
ax.plot(map_insert['avg'], map_insert['size'], label='map<>')
ax.plot(bst_insert['avg'], bst_insert['size'], label='bst<>')
plt.legend()
fig.savefig('./size.png')
plt.close(fig)


fig, ax = plt.subplots(nrows=1, ncols=1)
ax.set_xlabel('samples [ul]')
ax.set_ylabel('depth of the tree')
ax.plot(bst_insert['avg'], bst_insert['depth'], label='bst<>.depth()')
ax.plot(bst_insert['avg'], np.log2(bst_insert['avg']), label='log2')
ax.set_ylim(bottom=0)
plt.legend()
fig.savefig('./depth.png')
plt.close(fig)


# Depths
try:
    depths = pd.read_csv('./bst_depth.csv')
    depths['avg'] = (depths['upper'] + depths['lower']) / 2
except:
    depths = None

if depths is not None:
    _a0 = depths[depths['action'] == 0]
    _a1 = depths[depths['action'] == 1]
    _a2 = depths[depths['action'] == 2]

    fig, ax = plt.subplots(nrows=1, ncols=1)
    ax.set_xlabel('samples [ul]')
    ax.set_ylabel('depth of the tree')
    ax.plot(_a0['avg'], _a0['depth'], label='random', alpha=0.7)
    ax.plot(_a1['avg'], _a1['depth'], label='positive', alpha=0.7)
    ax.plot(_a2['avg'], _a2['depth'], label='negative', alpha=0.7)
    ax.plot(_a1['avg'], np.log2(_a1['avg']), label='log2', alpha=0.7)
    ax.set_ylim(bottom=0)
    plt.legend()
    fig.savefig('./all_depths.png')
    plt.close(fig)

