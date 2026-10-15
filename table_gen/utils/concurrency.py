"""Concurrency runners with graceful Ctrl+C handling and process-pool support."""

from collections.abc import Callable, Iterable, Iterator
from concurrent.futures import Future, ProcessPoolExecutor, ThreadPoolExecutor, as_completed
import multiprocessing as mp
import os
import signal
from typing import Any


def run_in_fork_process_pool[T, R](
    worker: Callable[[T], R],
    tasks: Iterable[T],
    max_workers: int = 1,
    initializer: Callable[..., None] | None = None,
    initargs: tuple[Any, ...] = (),
) -> Iterator[R]:
    """
    Execute tasks in a process pool using fork context (inheriting memory via CoW, bypassing GIL).

    Shuts down instantly on Ctrl+C.
    """
    if max_workers <= 1:
        if initializer is not None:
            initializer(*initargs)
        for task in tasks:
            yield worker(task)
        return

    ctx = mp.get_context("fork")
    executor = ProcessPoolExecutor(
        max_workers=max_workers,
        mp_context=ctx,
        initializer=initializer,
        initargs=initargs,
    )
    futures: list[Future[R]] = []
    try:
        futures = [executor.submit(worker, task) for task in tasks]
        for future in as_completed(futures):
            yield future.result()
    except KeyboardInterrupt:
        for f in futures:
            f.cancel()
        executor.shutdown(wait=False, cancel_futures=True)
        raise
    finally:
        executor.shutdown(wait=False, cancel_futures=True)


def run_in_thread_pool[T, R](
    worker: Callable[[T], R],
    tasks: Iterable[T],
    max_workers: int = 1,
) -> Iterator[R]:
    """
    Execute tasks in a thread pool with graceful shutdown on Ctrl+C.

    Yields results as each task completes.
    """
    if max_workers <= 1:
        for task in tasks:
            yield worker(task)
        return

    executor = ThreadPoolExecutor(max_workers=max_workers)
    futures: list[Future[R]] = []
    try:
        futures = [executor.submit(worker, task) for task in tasks]
        for future in as_completed(futures):
            yield future.result()
    except KeyboardInterrupt:
        for f in futures:
            f.cancel()
        executor.shutdown(wait=False, cancel_futures=True)
        raise
    finally:
        executor.shutdown(wait=False, cancel_futures=True)
