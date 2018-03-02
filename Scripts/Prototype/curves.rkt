#lang racket

(require racket/draw)
(require math/number-theory)

(require "flashplayer.rkt")

; https://pomax.github.io/bezierinfo/

(define (bezier-enclosing-box point others)
  (define-values (x0 y0) (values (real-part point) (imag-part point)))
  (let tr ([xmin x0] [ymin y0] [xmax x0] [ymax y0] [points others])
    (cond [(null? points) (values xmin ymin xmax ymax)]
          [else (let ([point (car points)])
                  (define x (real-part point))
                  (define y (imag-part point))
                  (tr (min xmin x) (min ymin y)
                      (max xmax x) (max ymax y)
                      (cdr points)))])))

(define (make-bezier-function point others)
  (define n (length others))
  (lambda [t]
    (let sum ([i 0] [points (cons point others)] [intermediate-point 0.0])
      (cond [(null? points) intermediate-point]
            [else (sum (+ i 1) (cdr points)
                       (+ intermediate-point
                          (* (binomial n i)
                             (expt (- 1.0 t) (- n i))
                             (expt t i)
                             (car points))))]))))

(define (make-point-transform x0 y0 xn yn width height margin)
  (define-values (x-range y-range) (values (- width margin margin) (- height margin margin)))
  (define-values (x-start y-start) (values (/ (- width x-range) 2) (- height margin)))
  (define-values (x-ratio y-ratio) (values (/ x-range (- xn x0)) (/ y-range (- yn y0))))
  (lambda [p]
    (cons (+ x-start (* (- (real-part p) x0) x-ratio))
          (- y-start (* (- (imag-part p) y0) y-ratio)))))

(define (in-bezier-range sample-count)
  (define step (/ 1 sample-count))
  (in-range 0 (+ 1 step) step))

(define (bezier-range sample-count)
  (sequence->list (in-bezier-range sample-count)))

(define (draw-endpoint dc point diameter)
  (define radius (* diameter 0.5))
  (send dc draw-ellipse
        (- (car point) radius)
        (- (cdr point) radius)
        diameter diameter))

(define (bezier #:width [width 256] #:height [height 256] #:samples [samples #false] #:dot-diameter [diameter 4.0] point . others)
  (define-values (x0 y0 xn yn) (bezier-enclosing-box point others))
  (define sample-count (or samples (max width height)))
  (define point-transform (make-point-transform x0 y0 xn yn width height diameter))
  (define make-intermediate-point (make-bezier-function point others))
  (define curve-nodes (for/list ([t (in-bezier-range sample-count)]) (point-transform (make-intermediate-point t))))
  (define point-nodes (map point-transform (cons point others)))
  (define canvas (make-bitmap width height #:backing-scale 2.0))
  (define dc (send canvas make-dc))
  (send dc set-smoothing 'smoothed)
  (send dc set-pen (make-pen #:color "Gainsboro"))
  (send dc draw-lines point-nodes)
  (for ([control-point (in-list (take (cdr point-nodes) (sub1 (length others))))])
    (draw-endpoint dc control-point diameter))
  (send dc set-pen (make-pen #:color "Green"))
  (draw-endpoint dc (car point-nodes) diameter)
  (send dc set-pen (make-pen #:color "Red"))
  (draw-endpoint dc (last point-nodes) diameter)
  (send dc set-pen (make-pen #:color "Black"))
  (send dc draw-lines curve-nodes)
  canvas)

(define (bezier* #:width [width 512] #:height [height 512] #:samples [samples #false] #:fps [fps 60] #:dot-diameter [diameter 4.0] point . others)
  (define-values (x0 y0 xn yn) (bezier-enclosing-box point others))
  (define point-transform (make-point-transform x0 y0 xn yn width height diameter))
  (define make-intermediate-point (make-bezier-function point others))
  (define time-series (bezier-range (or samples (max width height))))
  (define point-nodes (map point-transform (cons point others)))
  (define curve-nodes (for/list ([t (in-list time-series)]) (point-transform (make-intermediate-point t))))
  (define curve-node-count (length curve-nodes))
  (define nodes-sample-count (* curve-node-count 2))
  (define time-samples (list->vector (append time-series (reverse time-series))))
  (define do-draw
    (λ [dc times]
      (define pt-idx (let ([i (remainder times nodes-sample-count)]) (if (< i curve-node-count) i (- nodes-sample-count i 1))))
      (define t (vector-ref time-samples (remainder times (vector-length time-samples))))
      (when (= pt-idx 0) (collect-garbage 'incremental))
      (send bezier-player set-label (format "Bézier Curve[t = ~a]" (~r t #:precision '(= 3))))
      (send dc set-pen (make-pen #:color "Gainsboro"))
      (send dc draw-lines (drop curve-nodes pt-idx))
      (send dc draw-lines point-nodes)
      (for ([control-point (in-list (take (cdr point-nodes) (sub1 (length others))))])
        (draw-endpoint dc control-point diameter))
      (send dc set-pen (make-pen #:color "PaleTurquoise"))
      (let de-Casteljau ([skeletons (cons point others)])
        (when (> (length skeletons) 1)
          (define next-round-skeletions
            (for/list ([p1 (in-list skeletons)]
                       [p2 (in-list (cdr skeletons))])
              (+ p1 (* (- p2 p1) t))))
          (send dc draw-lines (map point-transform next-round-skeletions))
          (de-Casteljau next-round-skeletions)))
      (send dc set-pen (make-pen #:color "Black"))
      (send dc draw-lines (take curve-nodes pt-idx))
      (draw-endpoint dc (list-ref curve-nodes pt-idx) diameter)
      (send dc set-pen (make-pen #:color "Green"))
      (draw-endpoint dc (car point-nodes) diameter)
      (send dc set-pen (make-pen #:color "Red"))
      (draw-endpoint dc (last point-nodes) diameter)))
  (define bezier-player (new flashplayer% [fps fps] [width width] [height height] [margin 8] [draw-frame do-draw]))
  (send* bezier-player (show #true) (center 'both)))

(time (bezier 100.0+75.0i  20.0+110.0i  70.0+155.0i))
(time (bezier 60.0+105.0i  75.0+30.0i   215.0+115.0i 140.0+160.0i))
(time (bezier 120.0+160.0i 35.0+200.0i  220.0+260.0i 220.0+40.0i))
(time (bezier 25.0+128.0i  102.4+230.4i 153.6+25.6i  230.4+128.0i))
(time (bezier 198.0+18.0i  34.0+57.0i   18.0+156.0i  221.0+90.0i
              186.0+177.0i 14.0+82.0i   12.0+236.0i  45.0+290.0i
              218.0+294.0i 248.0+188.0i))


#;(bezier* 120.0+160.0i 35.0+200.0i  220.0+260.0i 220.0+40.0i)

(bezier* 198.0+18.0i  34.0+57.0i   18.0+156.0i  221.0+90.0i
         186.0+177.0i 14.0+82.0i   12.0+236.0i  45.0+290.0i
         218.0+294.0i 248.0+188.0i)