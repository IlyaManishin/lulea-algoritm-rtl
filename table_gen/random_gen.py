import ipaddress
import random
from pathlib import Path
from config import BUILD_DIR

DEFAULT_ROUTE_COUNT = 4000
DEFAULT_OUTPUT_FILE = BUILD_DIR / "routes.txt"

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

PORT_RANGE = (1, 255)


def generate_routes(
    count: int = DEFAULT_ROUTE_COUNT,
    output_path: str | Path = DEFAULT_OUTPUT_FILE,
    mask_weights: dict[int, float] | None = None,
    seed: int | None = None,
) -> list[tuple[str, int]]:
    if seed is not None:
        random.seed(seed)

    weights_dict = mask_weights or DEFAULT_MASK_WEIGHTS
    masks = list(weights_dict.keys())
    probs = list(weights_dict.values())

    assigned_masks = random.choices(masks, weights=probs, k=count)

    routes = []
    seen = set()

    for mask in assigned_masks:
        while True:
            ip_int = random.randint(0x01000000, 0xE0000000)
            net = ipaddress.IPv4Network((ip_int, mask), strict=False)
            if net not in seen:
                seen.add(net)
                port_id = random.randint(*PORT_RANGE)
                routes.append((str(net), port_id))
                break

    out_file = Path(output_path)
    out_file.parent.mkdir(parents=True, exist_ok=True)

    with open(out_file, "w") as f:
        for prefix, port in routes:
            f.write(f"{prefix} {port}\n")

    return routes


def main():
    generate_routes()


if __name__ == "__main__":
    main()
