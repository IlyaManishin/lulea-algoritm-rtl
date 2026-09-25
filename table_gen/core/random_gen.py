import ipaddress

from pathlib import Path
import random

from gen_types import RouteRecord

DEFAULT_MASK_WEIGHTS = {
    **{mask: 0.25 / 15 for mask in range(1, 16)},
    16: 1.2,
    17: 0.8,
    18: 1.2,
    19: 2.3,
    20: 4.2,
    21: 5.0,
    22: 10.3,
    23: 10.5,
    24: 64.0,
    **{mask: 0.25 / 8 for mask in range(25, 33)},
}

IP_WORD_RANGE = (1, 255)


def generate_random_routes(
    count: int,
    output_path: str | Path | None = None,
    mask_weights: dict[int, float] | None = None,
    seed: int | None = None,
) -> list[RouteRecord]:
    if seed is not None:
        random.seed(seed)

    weights_dict = mask_weights or DEFAULT_MASK_WEIGHTS
    masks = list(weights_dict.keys())
    probs = list(weights_dict.values())

    assigned_masks = random.choices(masks, weights=probs, k=count)

    routes: list[RouteRecord] = []
    seen = set()

    for mask in assigned_masks:
        while True:
            ip_int = random.randint(0x01000000, 0xE0000000)
            net = ipaddress.IPv4Network((ip_int, mask), strict=False)
            if net not in seen:
                seen.add(net)
                # Next hop format matches real BGP IP addresses
                next_hop = f"10.0.{random.randint(*IP_WORD_RANGE)}.{random.randint(*IP_WORD_RANGE)}"
                routes.append(
                    RouteRecord(
                        prefix=str(net),
                        mask=mask,
                        next_hop=next_hop,
                    )
                )
                break

    if output_path is not None:
        filepath = Path(output_path)
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, "w", encoding="utf-8") as f:
            for route in routes:
                f.write(f"{route.prefix} {route.next_hop}\n")

    return routes
