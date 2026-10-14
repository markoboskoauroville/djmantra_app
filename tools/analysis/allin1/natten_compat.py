"""Pure-torch replacement for the old NATTEN functions All-In-One uses
(natten1dqkrpb, natten1dav, natten2dqkrpb, natten2dav), same window and
relative-position-bias rules as NATTEN 0.14's reference kernels."""
import torch


def _window_start(i, n, k, r, d):
    if d <= 1:
        return max(i - r, 0) + (i + r >= n) * (n - i - r - 1)
    ni = i - r * d
    if ni < 0:
        return i % d
    if i + r * d >= n:
        imodd = i % d
        a = (n // d) * d
        b = n - a
        if imodd < b:
            return n - b + imodd - 2 * r * d
        return a + imodd - k * d
    return ni


def _pb_start(i, n, k, r, d):
    if d <= 1:
        return r + (i < r) * (r - i) + (i + r >= n) * (n - i - 1 - r)
    if i - r * d < 0:
        return k - 1 - (i // d)
    if i + r * d >= n:
        return (n - i - 1) // d
    return r


_cache = {}


def _idx(n, k, d, device):
    key = (n, k, d, device)
    if key not in _cache:
        r = k // 2
        kv, pb = [], []
        for i in range(n):
            s, p = _window_start(i, n, k, r, d), _pb_start(i, n, k, r, d)
            kv.append([s + j * d for j in range(k)])
            pb.append([p + j for j in range(k)])
        _cache[key] = (torch.tensor(kv, device=device), torch.tensor(pb, device=device))
    return _cache[key]


def natten1dqkrpb(q, k, rpb, kernel_size, dilation):
    B, H, L, D = q.shape
    kv, pb = _idx(L, kernel_size, dilation, q.device)
    kn = k[:, :, kv]                                   # B H L K D
    attn = (q.unsqueeze(3) * kn).sum(-1)               # B H L K
    if rpb is not None:
        attn = attn + rpb[:, pb].unsqueeze(0)          # H L K
    return attn


def natten1dav(attn, v, kernel_size, dilation):
    B, H, L, D = v.shape
    kv, _ = _idx(L, kernel_size, dilation, v.device)
    return (attn.unsqueeze(-1) * v[:, :, kv]).sum(3)


def natten2dqkrpb(q, k, rpb, kernel_size, dilation):
    B, H, X, Y, D = q.shape
    kx, px = _idx(X, kernel_size, dilation, q.device)
    ky, py = _idx(Y, kernel_size, dilation, q.device)
    kn = k[:, :, kx[:, None, :, None], ky[None, :, None, :]]   # B H X Y Kx Ky D
    attn = (q[:, :, :, :, None, None, :] * kn).sum(-1)
    if rpb is not None:
        attn = attn + rpb[:, px[:, None, :, None], py[None, :, None, :]].unsqueeze(0)
    return attn.reshape(B, H, X, Y, kernel_size * kernel_size)


def natten2dav(attn, v, kernel_size, dilation):
    B, H, X, Y, D = v.shape
    kx, _ = _idx(X, kernel_size, dilation, v.device)
    ky, _ = _idx(Y, kernel_size, dilation, v.device)
    vn = v[:, :, kx[:, None, :, None], ky[None, :, None, :]]   # B H X Y Kx Ky D
    a = attn.reshape(B, H, X, Y, kernel_size, kernel_size, 1)
    return (a * vn).sum((4, 5))
