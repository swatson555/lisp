(define fold-left
  (lambda (f n xs)
    (if (= xs '()) n
        (fold-left f (f n (car xs)) (cdr xs)))))

(fold-left + 0 '(0 1 2 3))
(fold-left + 0 '(0 1 2 3 4 5))

(define fold-right
  (lambda (f n xs)
    (if (= xs '()) n
        (f (car xs) (fold-right f n (cdr xs))))))

(fold-right + 0 '(0 1 2 3))
(fold-right + 0 '(0 1 2 3 4 5))

(define builtin-+ +)
(define +
  (lambda xs
    (fold-left builtin-+ 0 xs)))

(+ 1 2 3 4)
